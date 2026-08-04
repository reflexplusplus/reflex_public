package com.reflexplusplus.reflex.sdk

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import androidx.activity.addCallback
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.color.DynamicColors
import com.google.android.material.textfield.TextInputEditText
import com.reflexplusplus.reflex.R

class ReflexTextInputActivity : AppCompatActivity() {
	companion object {
		private var parentActivity: ReflexActivity? = null
		private var text: String = ""
		private var selectionStart: Int = 0
		private var selectionEnd: Int = 0
		private val startNewActivity = { context: Context ->
			context.startActivity(Intent(context, ReflexTextInputActivity::class.java))
		}
		private var onDismissInput: () -> Unit = {}
		private var onRequireInput = startNewActivity

		fun dismissInput() {
			onDismissInput()
		}

		// Starts the activity or updates its content
		fun requireInput(parent: ReflexActivity, text: String, selectionStart: Int, selectionEnd: Int) {
			this.parentActivity = parent
			this.text = text
			this.selectionStart = selectionStart
			this.selectionEnd = selectionEnd
			this.onRequireInput(parent)
		}
	}

	private var hasResetedCallback = false

	override fun onCreate(savedInstanceState: Bundle?) {
		DynamicColors.applyToActivityIfAvailable(this) // Enables Material You

		super.onCreate(savedInstanceState)

		setContentView(R.layout.reflex_input_activity)

		// Apply system bar padding
		val editText = findViewById<TextInputEditText>(R.id.edit_text)
		val okButton = findViewById<Button>(R.id.ok_button)
		val cancelButton = findViewById<Button>(R.id.cancel_button)

		cancelButton.setOnClickListener {
			Companion.parentActivity = null
			resetOnSetParametersCallback()
			parentActivity?.onTextInputFinished()
			finish()
		}

		okButton.setOnClickListener {
			val parentActivity = Companion.parentActivity
			resetOnSetParametersCallback()
			parentActivity?.onTextInputUpdate(editText.text.toString(), editText.selectionStart, editText.selectionEnd)
			parentActivity?.onTextInputFinished()
			finish()
		}

		editText.requestFocus()
		onBackPressedDispatcher.addCallback {
			val parentActivity = Companion.parentActivity
			resetOnSetParametersCallback()
			parentActivity?.onTextInputFinished()
			finish()
		}

		onDismissInput = {
			finish()
		}
		onRequireInput = { _ ->
			editText.setText(Companion.text)
			editText.setSelection(Companion.selectionStart, Companion.selectionEnd)
		}
		onRequireInput(this)
	}

	override fun onDestroy() {
		resetOnSetParametersCallback()
		super.onDestroy()
	}

	private fun resetOnSetParametersCallback() {
		if (!hasResetedCallback) {
			hasResetedCallback = true
			onDismissInput = {}
			onRequireInput = Companion.startNewActivity
			Companion.parentActivity = null
		}
	}
}
