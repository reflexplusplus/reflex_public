package com.reflex_multimedia.reflex.sdk

import android.Manifest
import android.annotation.SuppressLint
import android.app.ActivityManager
import android.app.AlertDialog
import android.app.ComponentCaller
import android.app.NativeActivity
import android.content.ActivityNotFoundException
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.content.res.XmlResourceParser
import android.media.AudioAttributes
import android.media.AudioDeviceCallback
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.net.Uri
import android.os.Build
import android.os.Build.VERSION
import android.os.Bundle
import android.os.Debug
import android.os.Handler
import android.os.Looper
import android.provider.OpenableColumns
import android.provider.Settings
import android.text.Editable
import android.text.InputType
import android.text.TextWatcher
import android.util.Log
import android.view.KeyEvent
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.webkit.MimeTypeMap
import android.webkit.URLUtil
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.core.content.FileProvider
import androidx.core.graphics.Insets
import androidx.core.view.OnApplyWindowInsetsListener
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.charset.Charset
import java.util.Locale
import java.util.Timer
import java.util.concurrent.CountDownLatch
import kotlin.concurrent.timerTask
import kotlin.math.floor
import kotlin.math.min
import androidx.core.net.toUri
import java.net.URLConnection

open class ReflexActivity(private val filepathsResourceId: Int) : NativeActivity(), OnApplyWindowInsetsListener {
	companion object {
		private var appContext: Context? = null

		private fun isServiceRunning(serviceClass: Class<*>) =
			(appContext!!.getSystemService(ACTIVITY_SERVICE) as ActivityManager)
				.getRunningServices(Int.MAX_VALUE)
				.find { serviceClass.name == it.service.className } != null

		@JvmStatic @Suppress("unused") // Used from C++
		fun isBackgroundAudioServiceRunning() = isServiceRunning(BackgroundAudioService::class.java)

		@JvmStatic @Suppress("unused") // Used from C++
		fun getOperatingSystemVersion(): String {
			return "${VERSION.RELEASE} (${Build.DISPLAY})"
		}
	}

	private val kAccessModeOpenReadOnly = 1
	private val kAccessModeCreateNew = 2
	private val kAccessModeOpenReadWrite = 3

	private val kVirtualKeyboardInputNormal = 0
	private val kVirtualKeyboardInputEmail = 1
	private val kVirtualKeyboardInputPassword = 2
	private val kVirtualKeyboardInputNumber = 3
	private val kVirtualKeyboardInputPhoneNumber = 4
	private val kVirtualKeyboardInputURL = 5
	private val kVirtualKeyboardInputMultiLine = 6

	// Notifications for native processing
	private external fun onCodeReady(density: Int)
	private external fun onAudioDevicesChanged()
	private external fun onRecordingPermissionResult(granted: Boolean)
	private external fun onWindowInsetsChanged(interactable: IntArray)
	external fun onTextInputUpdate(text: String, selectionStart: Int, selectionEnd: Int)
	external fun onTextInputFinished()
	external fun onDeepLink(url: String)
	external fun onUpdateTheme()
	// Others
	private external fun devLog(s: String) // Warning: do not call before onCodeReady
	private external fun devWarn(s: String) // Warning: do not call before onCodeReady
	private external fun notifyResultFromOpenFileDialog(result: Array<ByteArray>?)
	private external fun notifyResultFromMessageBox(result: Int)
	private external fun imeInsertText(charsToErase: Int, charsToInsert: String)
	private var serviceIntent: Intent? = null

	private val audioManager by lazy {
		getSystemService(AUDIO_SERVICE) as AudioManager
	}
	private val clipboardManager by lazy {
		getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
	}
	private val inputMethodManager by lazy {
		getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
	}
	private var hasRecordingPermission: Boolean? = null
	private val paths = mutableMapOf<String, String>()
	private var userdataPath: String = ""
	private var userdocsPath: String = ""
	private val realScreenDensity: Float by lazy { resources.displayMetrics.density }
	// Round (1-1.6 => 1, 1.6-2.4 => 2, 2.4-3-4 => 3, etc.)
	protected val intScreenDensity: Int by lazy {
		if (realScreenDensity >= 1.6)
			floor(realScreenDensity + 0.6).toInt()
		else
			floor(realScreenDensity + 0.4).toInt()
	}
	private val kPermissionRequestCode = 1
	private val kLaunchForResultRequestCode = 0x1000
	private var resultHandlerCallback: (Int, Intent?) -> Unit = { _, _ -> }
	private lateinit var textInputField: EditText
	private lateinit var keyboardInputView: EditText
	private var textInputActive = false

	// MEMO: the background audio thread cannot be re-enabled while the app in the background,
	// so if a music player quickly stops and starts the audio while the app is in the background,
	// the service will be stopped and not restarted, leading to the audio being stopped by the system
	// shortly after. Delaying deactivation of the service fixes that.
	private val kStopBackgroundAudioServiceDelayMillis = 1000L
	private var bgAudioServiceJob: Job? = null
	private val bgAudioServiceScope = CoroutineScope(Dispatchers.Main + SupervisorJob())


	override fun onCreate(savedInstanceState: Bundle?) {
		appContext = this
		super.onCreate(savedInstanceState)

		setupAudioDeviceCallback()
		gatherUserPaths()

		userdataPath = paths["userdata"] ?: throw IllegalStateException("Your filepaths.xml should have a userdata path defined")
		userdocsPath = paths["userdocs"] ?: throw IllegalStateException("Your filepaths.xml should have a userdocs path defined")

		// Scaling is not supported for now, but easy to implement: https://developer.android.com/develop/ui/views/touch-and-input/gestures/scale
		onCodeReady(intScreenDensity)
		devLog("onCodeReady: density=${realScreenDensity}, rounded to $intScreenDensity")

		ViewCompat.setOnApplyWindowInsetsListener(window.decorView, this)

		// We need a view (even though nothing will be drawn on a NativeActivity) to show the software keyboard
		keyboardInputView = EditText(this)
		keyboardInputView.visibility = View.VISIBLE
		keyboardInputView.inputType = EditorInfo.TYPE_CLASS_TEXT or EditorInfo.TYPE_TEXT_FLAG_NO_SUGGESTIONS or EditorInfo.TYPE_TEXT_FLAG_MULTI_LINE
		keyboardInputView.setOnKeyListener(object : View.OnKeyListener {
			override fun onKey(v: View?, keyCode: Int, event: KeyEvent?): Boolean {
				if (keyCode == KeyEvent.KEYCODE_DEL && event?.action == KeyEvent.ACTION_DOWN) {
					imeInsertText(1, "")
					return true
				}
				return false
			}
		})
		keyboardInputView.addTextChangedListener(object : TextWatcher {
			override fun afterTextChanged(s: Editable?) {}
			override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}

			override fun onTextChanged(s: CharSequence?, cursorAt: Int, before: Int, count: Int) {
				if (s == null || count <= 0) return

				imeInsertText(0, s.substring(cursorAt, cursorAt + count))
			}
		})
		addContentView(keyboardInputView, ViewGroup.LayoutParams(1, 1))

		// For single-line editing
		textInputField = EditText(this)
		textInputField.visibility = View.VISIBLE // Invisible but active for IME
		textInputField.showSoftInputOnFocus = true
		textInputField.imeOptions = EditorInfo.IME_ACTION_DONE
		textInputField.maxLines = 1
		textInputField.inputType = InputType.TYPE_CLASS_TEXT
		textInputField.addTextChangedListener(object : TextWatcher {
			override fun afterTextChanged(s: Editable?) {}
			override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}

			override fun onTextChanged(s: CharSequence?, cursorAt: Int, before: Int, count: Int) {
				if (textInputActive) {
					onTextInputUpdate(s.toString(), cursorAt + count, cursorAt + count)
				}
			}
		})
		addContentView(textInputField, ViewGroup.LayoutParams(1, 1))

		if (intent?.data != null) {
			onDeepLink(intent.data.toString())
		}

	}


	override fun onConfigurationChanged(newConfig: Configuration) {
		super.onConfigurationChanged(newConfig)

		onUpdateTheme()
	}


	override fun onNewIntent(newIntent: Intent?) {
		super.onNewIntent(newIntent)

		if (newIntent?.data != null) {
			onDeepLink(newIntent.data.toString())
		}
	}


	protected open fun getApplicationLabel(): String {
		val implementedClassName = javaClass.getPackage()?.name
		return implementedClassName ?: localClassName
	}


	private fun launchForResult(intent: Intent, onResult: (Int, Intent?) -> Unit) {
		resultHandlerCallback = onResult
		startActivityForResult(intent, kLaunchForResultRequestCode)
	}


	override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?, caller: ComponentCaller) {
		if (requestCode == kLaunchForResultRequestCode) {
			val cb = resultHandlerCallback
			resultHandlerCallback = { _, _ -> }
			cb(resultCode, data)
		}

		super.onActivityResult(requestCode, resultCode, data, caller)
	}


	private fun gatherUserPaths() {
		val xpp = resources.getXml(filepathsResourceId)
		var inPaths = false
		while (xpp.eventType != XmlResourceParser.END_DOCUMENT) {
			when (xpp.eventType) {
				XmlResourceParser.START_TAG -> {
					if (xpp.name == "paths") {
						inPaths = true
					}
					else if (xpp.name == "files-path") {
						if (!inPaths) continue

						var name = ""
						var path = ""
						for (i in 0 until xpp.attributeCount) {
							when (xpp.getAttributeName(i)) {
								"path" -> path = xpp.getAttributeValue(i)
								"name" -> name = xpp.getAttributeValue(i)
							}
						}
						if (name.isNotEmpty() && path.isNotEmpty()) {
							paths[name] = File(filesDir, path).absolutePath
						}
					}
				}
				XmlResourceParser.END_TAG -> {
					if (xpp.name == "paths") {
						inPaths = false
					}
				}
			}

			xpp.next()
		}

		// Create the directories defined in res/xml/filepaths.xml
		paths.forEach { (_, path) ->
			val dir = File(path)
			if (!dir.exists()) {
				val dirsCreated = File(path).mkdirs()
				assert(dirsCreated)
			}
		}
	}


	@Suppress("Unused")
	private fun enableBackgroundAudioService(enabled: Boolean) {
		val serviceIntent = serviceIntent ?: Intent(this, BackgroundAudioService::class.java)

		if (enabled) {
			bgAudioServiceJob?.cancel()
			bgAudioServiceJob = null

			// Already running
			if (!isBackgroundAudioServiceRunning()) {
				startService(serviceIntent)
			}
		}
		else {
			// Stop already requested
			if (bgAudioServiceJob != null || !isBackgroundAudioServiceRunning()) return

			bgAudioServiceJob = bgAudioServiceScope.launch {
				delay(kStopBackgroundAudioServiceDelayMillis)
				if (isBackgroundAudioServiceRunning()) {
					stopService(serviceIntent)
				}
			}
		}
	}

	@Suppress("Unused")
	private fun launch(path: String): Boolean {
		// Website
		if (URLUtil.isValidUrl(path)) {
			val intent = Intent(Intent.ACTION_VIEW, path.toUri())

			try {
				startActivity(intent)
				return true
			} catch (e: ActivityNotFoundException) {
				devLog("$path: no application knows how to open URL")
				return false
			}
		}

		// File
		val file = File(path)

		if (!file.exists()) {
			devLog("$path: file not found")
			return false
		}

		// Try to infer MIME type from file name
		val mime = URLConnection.guessContentTypeFromName(file.name) ?: "*/*"

		val uri: Uri = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			// On Android 7.0+ you cannot share file:// URIs directly — use FileProvider instead
			try {
				FileProvider.getUriForFile(this, "${packageName}.fileprovider", file)
			} catch (e: IllegalArgumentException) {
				// Fallback if file is outside FileProvider’s allowed paths
				Uri.fromFile(file)
			}
		} else {
			Uri.fromFile(file)
		}

		val intent = Intent(Intent.ACTION_VIEW).apply {
			setDataAndType(uri, mime)
			addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
		}

		return try {
			startActivity(intent)
			true
		} catch (e: ActivityNotFoundException) {
			devLog("$path: no application knows how to open file")
			false
		}
	}


	private fun mimeTypeFromPath(path: String): String? {
		val extension = path.substringAfterLast('.', "").lowercase(Locale.ROOT)
		if (extension.isEmpty()) return null
		return MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension)
	}


	private fun uriMimeType(uri: Uri): String? {
		return contentResolver.getType(uri) ?: mimeTypeFromPath(uri.lastPathSegment ?: uri.toString())
	}


	private fun buildShareIntent(items: Array<String>, text: String?): Intent? {
		if (items.isEmpty()) return null

		val streamUris = ArrayList<Uri>()
		val textParts = ArrayList<String>()
		if (!text.isNullOrEmpty()) {
			textParts.add(text)
		}

		var mimeType: String? = null
		var forceWildcard = false

		fun registerMimeType(candidate: String?) {
			val normalized = candidate?.takeIf { it.isNotEmpty() } ?: "*/*"
			if (mimeType == null) {
				mimeType = normalized
			} else if (mimeType != normalized) {
				forceWildcard = true
			}
		}

		for (item in items) {
			if (File(item).isAbsolute) {
				val file = File(item)
				if (!file.exists() || file.isDirectory) return null

				val uri = try {
					FileProvider.getUriForFile(this, "${packageName}.fileprovider", file)
				} catch (e: IllegalArgumentException) {
					devLog("$item: FileProvider could not create a share URI")
					return null
				}

				streamUris.add(uri)
				registerMimeType(mimeTypeFromPath(file.name))
				continue
			}

			val uri = Uri.parse(item)
			val scheme = uri.scheme?.lowercase(Locale.ROOT) ?: return null

			when (scheme) {
				"content", "android.resource" -> {
					streamUris.add(uri)
					registerMimeType(uriMimeType(uri))
				}

				"file" -> {
					val filePath = uri.path ?: return null
					val file = File(filePath)
					if (!file.exists() || file.isDirectory) return null

					val contentUri = try {
						FileProvider.getUriForFile(this, "${packageName}.fileprovider", file)
					} catch (e: IllegalArgumentException) {
						devLog("$item: FileProvider could not create a share URI")
						return null
					}

					streamUris.add(contentUri)
					registerMimeType(mimeTypeFromPath(file.name))
				}

				else -> {
					textParts.add(item)
				}
			}
		}

		val shareText = textParts.takeIf { it.isNotEmpty() }?.joinToString("\n")
		val resolvedMimeType = if (streamUris.size > 1 || forceWildcard) "*/*" else mimeType ?: "*/*"

		val intent = when {
			streamUris.size > 1 -> Intent(Intent.ACTION_SEND_MULTIPLE).apply {
				type = resolvedMimeType
				putParcelableArrayListExtra(Intent.EXTRA_STREAM, streamUris)
			}

			streamUris.size == 1 -> Intent(Intent.ACTION_SEND).apply {
				type = resolvedMimeType
				putExtra(Intent.EXTRA_STREAM, streamUris[0])
			}

			!shareText.isNullOrEmpty() -> Intent(Intent.ACTION_SEND).apply {
				type = "text/plain"
			}

			else -> return null
		}

		if (!shareText.isNullOrEmpty()) {
			intent.putExtra(Intent.EXTRA_TEXT, shareText)
		}

		if (streamUris.isNotEmpty()) {
			val clipData = ClipData.newUri(contentResolver, getApplicationLabel(), streamUris[0])
			for (i in 1 until streamUris.size) {
				clipData.addItem(ClipData.Item(streamUris[i]))
			}
			intent.clipData = clipData
		}

		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
		return intent
	}


	private fun shareOnUiThread(items: Array<String>, text: String?): Boolean {
		val intent = buildShareIntent(items, text) ?: return false
		val chooser = Intent.createChooser(intent, null)

		if (chooser.resolveActivity(packageManager) == null) {
			return false
		}

		return try {
			startActivity(chooser)
			true
		} catch (e: ActivityNotFoundException) {
			devLog("No application available to share the requested content")
			false
		}
	}


	@Suppress("unused") // Used from C++
	fun share(items: Array<String>, text: String?): Boolean {
		if (items.isEmpty()) return false

		if (Looper.myLooper() == Looper.getMainLooper()) {
			return shareOnUiThread(items, text)
		}

		val latch = CountDownLatch(1)
		var result = false

		runOnUiThread {
			result = shareOnUiThread(items, text)
			latch.countDown()
		}

		return try {
			latch.await()
			result
		} catch (e: InterruptedException) {
			Thread.currentThread().interrupt()
			false
		}
	}


	@Suppress("unused") // Used from C++
	fun getLanguage(): String =
		if (VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			resources.configuration.locales.get(0).language // first preferred locale
		} else {
			resources.configuration.locale.language
		}


	@Suppress("unused") // Used from C++
	fun isDarkTheme(): Boolean {
		val nightModeFlags: Int = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK
		return nightModeFlags == Configuration.UI_MODE_NIGHT_YES
	}


	@Suppress("unused") // Used from C++
	fun getFontSize(): Float {
		return resources.configuration.fontScale
	}


	@Suppress("unused") // Used from C++
	fun showKeyboard(shown: Boolean) {
		runOnUiThread {
			val insetsController = WindowCompat.getInsetsController(window, keyboardInputView)
			keyboardInputView.text.clear()
			if (shown) {
				insetsController.show(WindowInsetsCompat.Type.ime())
			}
			else {
				insetsController.hide(WindowInsetsCompat.Type.ime())
			}
		}
	}


	@Suppress("unused") // Used from C++
	fun enableTextInput(enable: Boolean, textType: Int, text: String, selectionStart: Int, selectionEnd: Int) {
		if (textType == kVirtualKeyboardInputMultiLine) {
			if (enable) {
				ReflexTextInputActivity.requireInput(this, text, selectionStart, selectionEnd)
			}
			else {
				ReflexTextInputActivity.dismissInput()
			}
			return
		}

		runOnUiThread {
			if (enable) {
				textInputActive = false	// textInputField.setText triggers the onTextChanged callback
				textInputField.inputType = when (textType) {
					kVirtualKeyboardInputNormal -> InputType.TYPE_CLASS_TEXT
					kVirtualKeyboardInputURL -> InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_URI
					kVirtualKeyboardInputEmail -> InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS
					kVirtualKeyboardInputNumber -> InputType.TYPE_CLASS_NUMBER
					kVirtualKeyboardInputPassword -> InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_PASSWORD
					kVirtualKeyboardInputPhoneNumber -> InputType.TYPE_CLASS_PHONE
					else -> InputType.TYPE_CLASS_TEXT
				}
				textInputField.setText(text)
				textInputField.setSelection(selectionStart, selectionEnd)
				textInputField.requestFocus()
				inputMethodManager.showSoftInput(textInputField, InputMethodManager.SHOW_IMPLICIT)
				textInputActive = true
			}
			else {
				textInputActive = false
				textInputField.text.clear()
				textInputField.clearFocus()
				inputMethodManager.hideSoftInputFromWindow(textInputField.windowToken, 0)
				window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN)
			}
		}
	}


	private fun hasRecordingPermission(): Boolean {
		if (hasRecordingPermission == null) {
			hasRecordingPermission = checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
		}
		return hasRecordingPermission!!
	}


	@Suppress("unused") // Used from C++
	fun requestRecordingPermission(): Boolean {
		val result = hasRecordingPermission()
		if (!result) {
			requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO), kPermissionRequestCode)
		}
		return result
	}


	override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
		if (requestCode == kPermissionRequestCode) {
			// Check if the permission was granted
			if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
				hasRecordingPermission = true
				onRecordingPermissionResult(true)
			}
			else {
				Log.d(this.javaClass.canonicalName, "Permission for audio recording has been refused, inputs will not be presented")
				hasRecordingPermission = false
				onRecordingPermissionResult(false)
			}
		}

		super.onRequestPermissionsResult(requestCode, permissions, grantResults)
	}


	override fun onApplyWindowInsets(v: View, insets: WindowInsetsCompat): WindowInsetsCompat {
		Handler(Looper.getMainLooper()).post {
			val totalInsets = Insets.max(
				insets.getInsets(WindowInsetsCompat.Type.systemBars()),
				insets.getInsets(WindowInsetsCompat.Type.ime()))
			onWindowInsetsChanged(intArrayOf(totalInsets.left, totalInsets.top, totalInsets.right, totalInsets.bottom))
		}

		return insets
	}


	@Suppress("unused") // Used from C++
	fun isDebuggerAttached(): Boolean {
		return Debug.isDebuggerConnected()
	}


	private fun setupAudioDeviceCallback() {
		// Note that we will immediately receive a call to onDevicesAdded with the list of
		// devices which are currently connected.
		audioManager.registerAudioDeviceCallback(object : AudioDeviceCallback() {
			override fun onAudioDevicesAdded(addedDevices: Array<AudioDeviceInfo>) {
				Timer().schedule(timerTask { onAudioDevicesChanged() }, 0)
			}

			override fun onAudioDevicesRemoved(removedDevices: Array<AudioDeviceInfo>) {
				Timer().schedule(timerTask { onAudioDevicesChanged() }, 0)
			}
		}, null)
	}


	@Suppress("unused") // Used from C++
	fun getAudioDevices(defaultDeviceName: String): Array<AudioDevice> {
		val audioDevices = audioManager.getDevices(AudioManager.GET_DEVICES_INPUTS or AudioManager.GET_DEVICES_OUTPUTS)
		val defaultOutputDevices = findDefaultOutputDevices()
		var allSampleRates = setOf(AudioDevice.DEFAULT_SAMPLE_RATE)
		val specificDevices = audioDevices
			.filter {
				when (it.type) {
					// Those types of devices cannot be used
					AudioDeviceInfo.TYPE_TELEPHONY, AudioDeviceInfo.TYPE_REMOTE_SUBMIX,
					28, 29, AudioDeviceInfo.TYPE_BUILTIN_SPEAKER_SAFE -> false
					else -> it.isSource || it.isSink
				}
			}
			.onEach { allSampleRates = allSampleRates union it.sampleRates.toSet() }
			.map { AudioDevice(it, defaultOutputDevices) }

		val addDefault = specificDevices.find { it.isDefault } == null
		val sampleRates = allSampleRates.sorted().toIntArray()
		// Provide default device &&
		return arrayOf(
			AudioDevice(defaultDeviceName, false, addDefault, sampleRates),
			AudioDevice(defaultDeviceName, true, addDefault, sampleRates)) + specificDevices
	}


	@SuppressLint("InlinedApi")
	private fun findDefaultOutputDevices(): List<AudioDeviceInfo> {
		// Recent Android versions have a proper way of detecting the best output device
		if (VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
			// Might return an empty list when a bluetooth A2DP device is present
			audioManager.getAudioDevicesForAttributes(
				AudioAttributes.Builder().setUsage(
					AudioAttributes.USAGE_MEDIA).build())
				.let { if (it.isNotEmpty()) return@findDefaultOutputDevices it }
		}

		// Older android versions
		val devices = audioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS).filter { it.isSink }
		if (devices.isEmpty()) return devices

		// Priority determined arbitrarily
		val deviceTypes = arrayOf(
			AudioDeviceInfo.TYPE_WIRED_HEADPHONES,
			AudioDeviceInfo.TYPE_USB_DEVICE,
			AudioDeviceInfo.TYPE_BLUETOOTH_A2DP, // high quality bluetooth
			AudioDeviceInfo.TYPE_USB_HEADSET,
			AudioDeviceInfo.TYPE_WIRED_HEADSET,
			AudioDeviceInfo.TYPE_AUX_LINE,
			AudioDeviceInfo.TYPE_BLE_SPEAKER,
			AudioDeviceInfo.TYPE_BLE_HEADSET,
			AudioDeviceInfo.TYPE_USB_ACCESSORY,
			AudioDeviceInfo.TYPE_BLUETOOTH_SCO, // lower quality, but if it's connected and we don't have anything better, let's still use this over the built-in speakers
			AudioDeviceInfo.TYPE_BUILTIN_SPEAKER,
		)

		deviceTypes.forEach { type ->
			devices.filter { it.type == type }.let {
				if (it.isNotEmpty()) return@findDefaultOutputDevices it
			}
		}

		// Otherwise just pick the first that's available
		return listOf(devices.first())
	}


	@Suppress("unused") // Used from C++
	fun getApplicationDir(): String {
		return applicationInfo.dataDir
	}


	@Suppress("unused") // Used from C++
	fun getUserdataDir(): String {
		return userdataPath
	}


	@Suppress("unused") // Used from C++
	fun getUserdocsDir(): String {
		return userdocsPath
	}


	@Suppress("unused") // Used from C++
	fun getTempDir(): String {
		return cacheDir.absolutePath
	}


	@Suppress("unused") // used from C++
	fun showMessageBox(title: String, text: String) {
		runOnUiThread {
			var result = 0
			AlertDialog.Builder(this)
				.setTitle(title)
				.setMessage(text)
				.setCancelable(true)
				// Specifying a listener allows you to take an action before dismissing the dialog.
				// The dialog is automatically dismissed when a dialog button is clicked.
				.setPositiveButton(
					android.R.string.ok,
					DialogInterface.OnClickListener { dialog, which ->
						run {
							result = 1 // IDOK
							dialog.dismiss()
						}
					})
				.setOnDismissListener {
					notifyResultFromMessageBox(result)
				}
				.setIcon(android.R.drawable.ic_dialog_info)
				.show()
		}
	}


	private fun allUrisInResult(resultData: Intent?): Array<Uri>? {
		if (resultData?.data != null) { // Single file selected
			return arrayOf(resultData.data!!)
		}
		else if (resultData?.clipData != null) { // Multiple file selected
			val clipData = resultData.clipData!!
			return Array(clipData.itemCount) {
				i -> clipData.getItemAt(i).uri
			}
		}
		else {
			return null
		}
	}


	private fun buildFilenameWithExtension(fileName: String, mimeTypes: Array<String>): String {
		if (mimeTypes.isNotEmpty()) {
			val ext = MimeTypeMap.getSingleton().getExtensionFromMimeType(mimeTypes[0])
			if (!ext.isNullOrEmpty()) {
				return "$fileName.$ext"
			}
		}
		return fileName
	}


	@Suppress("unused") // Used from C++
	fun showFilePicker(mimeTypes: Array<String>, accessType: Int, allowMultiple: Boolean, suggestedFileName: String?) {
		val permission =
			if (accessType == kAccessModeOpenReadOnly) {
				Intent.FLAG_GRANT_READ_URI_PERMISSION
			} else {
				Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
			}
		val intent =
			if (accessType == kAccessModeCreateNew) {
				Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
					if (!suggestedFileName.isNullOrEmpty()) {
						putExtra(Intent.EXTRA_TITLE, buildFilenameWithExtension(suggestedFileName, mimeTypes))
					}
				}
			} else {
				Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
					putExtra(Intent.EXTRA_ALLOW_MULTIPLE, allowMultiple)
				}
			}

		intent.apply {
			addCategory(Intent.CATEGORY_OPENABLE)
			when (mimeTypes.size) {
				0 -> type = "*/*"
				1 -> type = mimeTypes[0]
				2 -> if (accessType == kAccessModeCreateNew) {
					type = mimeTypes[0]
					putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes.sliceArray(1 until mimeTypes.size))
				} else {
					type = "*/*"
					putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes)
				}
			}
		}

		launchForResult(intent) callback@{ resultCode, data ->
			if (resultCode != RESULT_OK) {
				notifyResultFromOpenFileDialog(null)
				return@callback
			}

			val fileUris = allUrisInResult(data) ?: return@callback
			for (uri in fileUris) {
				contentResolver.takePersistableUriPermission(uri, permission)
			}

			val charset = Charset.forName("UTF-8")
			val transformedToString = Array<ByteArray>(fileUris.size) {
				i -> fileUris[i].toString().toByteArray(charset)
			}
			notifyResultFromOpenFileDialog(transformedToString)
		}
	}


	@Suppress("unused") // Used from C++
	fun openInputStream(fileUri: Uri): InputStream? {
		return contentResolver.openInputStream(fileUri)
	}


	@Suppress("unused") // Used from C++
	fun getFileSize(fileUri: Uri): Long {
//		return contentResolver.openFile(fileUri, "r", null)?.statSize
		val cursor = contentResolver.query(fileUri, null, null, null, null)
		if (cursor != null) {
			cursor.moveToFirst()
			val colIndex = cursor.getColumnIndex(OpenableColumns.SIZE)
			if (colIndex >= 0) {
				return cursor.getLong(colIndex)
			}
			cursor.close()
		}
		return -1
	}


	@Suppress("unused") // Used from C++
	fun readFileFully(fileUri: Uri, result: ByteBuffer) {
		val inputStream = contentResolver.openInputStream(fileUri)
		if (inputStream != null) {
			val buf = ByteArray(4096)
			while (inputStream.available() > 0) {
				val count = inputStream.read(buf)
				result.put(buf, 0, count)
			}
			inputStream.close()
		}
	}


	@Suppress("unused") // Used from C++
	fun writeFileFully(fileUri: Uri, data: ByteBuffer) {
		val pfd = contentResolver.openFileDescriptor(fileUri, "w") ?: return
		val fileOutputStream = FileOutputStream(pfd.fileDescriptor)
		val buf = ByteArray(4096)
		var written: Long = 0
		while (data.hasRemaining()) {
			val count = min(buf.size, data.remaining())
			data.get(buf, 0, count)
			fileOutputStream.write(buf, 0, count)
			written += count
		}
		// Use this code to ensure that the file is 'emptied'
		// https://stackoverflow.com/questions/56902845/how-to-properly-overwrite-content-of-file-using-android-storage-access-framework
		fileOutputStream.channel.truncate(written)
		fileOutputStream.close()
		pfd.close()
	}


	@Suppress("unused") // Used from C++
	fun copyTextToClipboard(text: String) {
		val clip = ClipData.newPlainText(getApplicationLabel(), text)
		clipboardManager.setPrimaryClip(clip)
	}


	@Suppress("unused") // Used from C++
	fun getTextFromClipboard(): String? {
		val clipData: ClipData? = clipboardManager.primaryClip
		val item = clipData?.getItemAt(0)
		return item?.text?.toString()
	}


	@Suppress("unused") // Used from C++
	fun getNumProcessors(): Int {
		return Runtime.getRuntime().availableProcessors()
	}


	@Suppress("unused") // Used from C++
	fun getAndroidId(): String {
		return Settings.Secure.getString(contentResolver, Settings.Secure.ANDROID_ID);
	}
}
