#pragma once

#include "[require].h"
#include "../../common/utf8.h"

// Classes that are defined in Java, wrapper
REFLEX_NS(Reflex::System::Android::Jni)

// No corresponding instances for those because they're little used and there would be a small perf overhead
struct StringClass : Object {
	explicit StringClass(JNIEnv* env)
		: cls(env->FindClass("java/lang/String"))
		, utf8(env->NewStringUTF("UTF-8"))
		, constructor(env->GetMethodID(cls.ref, "<init>", MethodSignature<void, Array<UInt8>, CString>()))
		, getBytes(env->GetMethodID(cls.ref,  "getBytes", MethodSignature<Array<UInt8>, CString>()))
	{
		cls.MakeGlobalRef(env);
		utf8.MakeGlobalRef(env);
	}

	void New(JNIEnv* env, const Array<UInt8>& utf8Data, JavaRef<jobject>& out) {
		out.SetAsOwned(env->NewObject(cls.ref, constructor, JavaRef(env, utf8Data).ref, utf8.ref));
	}

	void GetBytes(JNIEnv* env, jobject instance, Array<char>& out_result) {
		JavaRef<jbyteArray> stringJbytes((jbyteArray)env->CallObjectMethod(instance, getBytes, utf8.ref));
		AssertNothingThrown(env);

		auto length = (size_t) env->GetArrayLength(stringJbytes);
		jbyte *pBytes = env->GetByteArrayElements(stringJbytes, nullptr);

		out_result.SetSize(length);
		MemCopy(pBytes, out_result.GetData(), length);
		env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);
	}

	JavaRef<jclass> cls;
	JavaRef<jstring> utf8;
	jmethodID constructor, getBytes;
};

struct ExceptionClass : Object {
	explicit ExceptionClass(JNIEnv* env)
		: cls(env->FindClass("java/lang/Exception"))
		, getMessage(env->GetMethodID(cls.ref, "getMessage", MethodSignature<CString>()))
		, printStackTrace(env->GetMethodID(cls.ref, "printStackTrace", MethodSignature<void>()))
	{
		cls.MakeGlobalRef(env);
	}

	CString GetMessage(JNIEnv* env, jobject instance) const {
		JavaRef msg { jstring(env->CallObjectMethod(instance, getMessage)) };
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			return "(error while getting message)";
		}
		return msg.ToNative<CString>(env);
	}

	void PrintStackTrace(JNIEnv* env, jobject instance) const {
		env->CallVoidMethod(instance, printStackTrace);
		if (env->ExceptionCheck()) {
			DEV_LOG("(stack trace cannot be printed)");
			env->ExceptionClear();
		}
	}

	JavaRef<jclass> cls;
	jmethodID getMessage, printStackTrace;
};

struct UriClass : Object {
	explicit UriClass(JNIEnv* env, StringClass& stringClass_);

	void Parse(JNIEnv* env, const Array<char>& utf8FileUri, JavaRef<jobject>& out) {
		JavaExceptionCheckScope scope(env);
		JavaRef fname(env, utf8FileUri);

		out.SetAsOwned(env->CallStaticObjectMethod(cls, parse, fname.ref));
	}

	void FromUTF8(JNIEnv* env, const Array<UInt8>& in, JavaRef<jobject>& out) {
		JavaRef string;

		stringClass.New(env, in, string);
		AssertNothingThrown(env);

		out.SetAsOwned(env->CallStaticObjectMethod(cls, parse, string.ref));
	}

	// Bad performance, for debugging
	CString ToString(JNIEnv* env, JavaRef<jobject>& instance) {
		JavaExceptionCheckScope scope(env);
		jmethodID toString = env->GetMethodID(cls.ref, "toString", MethodSignature<CString>());
		scope.AssertOk();

		JavaRef result { jstring(env->CallObjectMethod(instance, toString)) };
		return scope.CheckAndClearException()
			? CString()
			: result.ToNative<CString>(env);
	}

private:
	JavaRef<jclass> cls;
	jmethodID parse;
	StringClass& stringClass;
};

template<> inline const char* JavaSignature<UriClass> = "Landroid/net/Uri;";

struct InputStreamClass : Object {

	explicit InputStreamClass(JNIEnv* env) {
		// We don't use findClass for system classes
		JavaRef clazz (env->FindClass("java/io/InputStream"));
		read = env->GetMethodID(clazz.ref, "read", MethodSignature<int, Array<UInt8>, int, int>());
		skip = env->GetMethodID(clazz.ref, "skip", MethodSignature<Int64, Int64>());
		available = env->GetMethodID(clazz.ref, "available", MethodSignature<int>());
		close = env->GetMethodID(clazz.ref, "close", MethodSignature<void>());
	}

	jmethodID read, skip, available, close;
};

template<> inline const char* JavaSignature<InputStreamClass> = "Ljava/io/InputStream;";
template<> struct ReflexTypeToJava<InputStreamClass> { using type = jobject; };


template<bool THROWS_EXCEPTIONS>
struct InputStreamInstance : Object {

	// Note: the ownership of inputStream will be taken over by InputStreamInstance
	InputStreamInstance(JNIEnv* env_, JavaRef<jobject>& inputStream)
		: instance(inputStream.DetachOwnership(), inputStream.IsGlobalRef())
		, env(env_)
	{
		if (!instance.IsGlobalRef()) instance.MakeGlobalRef(env);
	}

	int Read(const JavaRef<jbyteArray>& buf, int offset, int length) {
		if (!instance) return 0;
		auto result = env->CallIntMethod(instance, cls().read, buf.ref, offset, length);
		HandlePossibleException();
		return result;
	}

	Int64 Skip(Int64 n) {
		if (!instance) return 0;
		auto result = env->CallLongMethod(instance, cls().skip, n);
		HandlePossibleException();
		return result;
	}

	int Available() {
		if (!instance) return 0;
		auto result = env->CallIntMethod(instance, cls().available);
		HandlePossibleException();
		return result;
	}

	void Close() {
		if (!instance) return;
		env->CallVoidMethod(instance, cls().close);
		HandlePossibleException();
	}

	void Reopen(const JavaRef<jobject>& fileUri);

	explicit operator bool() const { return instance; }

private:
	JNIEnv* env;
	JavaRef<jobject> instance;

	bool HandlePossibleException() {
		if constexpr (THROWS_EXCEPTIONS) {
			AssertNothingThrown(env);
		}
		return JavaExceptionCheckScope(env).CheckAndClearException();
	}

	static inline const InputStreamClass& cls();
};

struct OutputStreamClass : Object {
	// We don't use findClass for system classes
	explicit OutputStreamClass(JNIEnv* env)
		: OutputStreamClass(env, JavaRef(env->FindClass("java/io/OutputStream"))) {}

	OutputStreamClass(JNIEnv* env, jclass clazz)
		: write(env->GetMethodID(clazz, "write", MethodSignature<void, Array<UInt8>, int, int>()))
		, flush(env->GetMethodID(clazz, "flush", MethodSignature<void>()))
		, close(env->GetMethodID(clazz, "close", MethodSignature<void>()))
	{}

	jmethodID write, flush, close;
};

template<bool THROWS_EXCEPTIONS>
struct OutputStreamInstance : Object {

	// Note: the ownership of inputStream will be taken over by InputStreamInstance
	OutputStreamInstance(JNIEnv* env_, JavaRef<jobject>& outputStream)
		: instance(outputStream.DetachOwnership(), outputStream.IsGlobalRef())
		, env(env_)
	{
		if (!instance.IsGlobalRef()) instance.MakeGlobalRef(env);
	}

	bool Write(const JavaRef<jbyteArray>& buf, int offset, int length) {
		if (!instance) return false;
		env->CallVoidMethod(instance, cls().write, buf.ref, offset, length);
		return !HandlePossibleException();
	}

	void Flush() {
		if (!instance) return;
		env->CallVoidMethod(instance, cls().flush);
		HandlePossibleException();
	}

	void Close() {
		if (!instance) return;
		env->CallVoidMethod(instance, cls().close);
		HandlePossibleException();
		instance.SetAsOwned(nullptr);
	}

	explicit operator bool() const { return instance; }

private:
	JNIEnv* env;
	JavaRef<jobject> instance;

	bool HandlePossibleException() {
		JavaExceptionCheckScope scope(env);
		if constexpr (THROWS_EXCEPTIONS) {
			scope.AssertOk();
		}
		return scope.CheckAndClearException();
	}

	static inline const OutputStreamClass& cls();
};

struct AudioDeviceInstance;
struct AudioDeviceClass;
template<> inline const char* JavaSignature<Array<AudioDeviceClass>> = "[Lcom/reflex_multimedia/reflex/sdk/AudioDevice;";
template<> struct ReflexTypeToJava<Array<AudioDeviceClass>> { using type = jobjectArray ; };


struct ReflexActivityClass {
	ReflexActivityClass(JNIEnv* env, jclass activityClass)
		: cls(env, activityClass)
		, getApplicationDir(env, cls, "getApplicationDir")
		, getUserdataDir(env, cls, "getUserdataDir")
		, getUserdocsDir(env, cls, "getUserdocsDir")
		, getTempDir(env, cls, "getTempDir")
		, getAudioDevices(env, cls, "getAudioDevices")
		, requestRecordingPermission(env, cls, "requestRecordingPermission")
		, isDebuggerAttached(env, cls, "isDebuggerAttached")
		, openInputStream(env, cls, "openInputStream")
		, getFileSize(env, cls, "getFileSize")
		, readFileFully(env, cls, "readFileFully")
		, writeFileFully(env, cls, "writeFileFully")
		, showMessageBox(env, cls, "showMessageBox")
		, showFilePicker(env, cls, "showFilePicker")
		, copyTextToClipboard(env, cls, "copyTextToClipboard")
		, getTextFromClipboard(env, cls, "getTextFromClipboard")
		, getNumProcessors(env, cls, "getNumProcessors")
		, enableBackgroundAudioService(env, cls, "enableBackgroundAudioService")
		, finishActivity(env, cls, "finishActivity")
		, showKeyboard(env, cls, "showKeyboard")
		, enableTextInput(env, cls, "enableTextInput")
		, setRequestedOrientation(env, cls, "setRequestedOrientation")
		, getAndroidId(env, cls, "getAndroidId")
		, isDarkTheme(env, cls, "isDarkTheme")
		, getFontSize(env, cls, "getFontSize")
		, getLanguage(env, cls, "getLanguage")
		, launch(env, cls, "launch")
		, share(env, cls, "share")
		, isBackgroundAudioServiceRunning(env, cls, "isBackgroundAudioServiceRunning")
		, getOperatingSystemVersion(env, cls, "getOperatingSystemVersion")
	{}

	GlobalJavaRef <jclass> cls;

	JavaMethod <CString> getApplicationDir;
	JavaMethod <CString> getUserdataDir;
	JavaMethod <CString> getUserdocsDir;
	JavaMethod <CString> getTempDir;
	JavaMethod <Array<AudioDeviceClass>, CString> getAudioDevices;
	JavaMethod <bool> requestRecordingPermission;
	JavaMethod <bool> isDebuggerAttached;
	JavaMethod <InputStreamClass, UriClass> openInputStream;
	JavaMethod <Int64, UriClass> getFileSize;
	JavaMethod <void, UriClass, void*> readFileFully;
	JavaMethod <void, UriClass, void*> writeFileFully;
	JavaMethod <void, CString, CString> showMessageBox;
	JavaMethod <void, Array<CString>, int, bool, CString> showFilePicker;
	JavaMethod <void, CString> copyTextToClipboard;
	JavaMethod <CString> getTextFromClipboard;
	JavaMethod <int> getNumProcessors;
	JavaMethod <void, bool> enableBackgroundAudioService;
	JavaMethod <void, int> finishActivity;
	JavaMethod <void, bool> showKeyboard;
	JavaMethod <void, bool, int, CString, int, int> enableTextInput;
	JavaMethod <void, int> setRequestedOrientation;
	JavaMethod <CString> getAndroidId;
	JavaMethod <bool> isDarkTheme;
	JavaMethod <float> getFontSize;
	JavaMethod <CString> getLanguage;
	JavaMethod <bool, CString> launch;
	JavaMethod <bool, Array<CString>, CString> share;

	JavaStaticMethod<bool> isBackgroundAudioServiceRunning;
	JavaStaticMethod<CString> getOperatingSystemVersion;

	bool IsBackgroundAudioServiceRunning(JNIEnv* env) const;

	CString GetOperatingSystemVersion(JNIEnv* env) const;
};

struct ReflexActivityInstance : Object {
	ReflexActivityInstance(JNIEnv* env_, jobject instance_)
		: exceptionClass(env_)
		, inputStreamClass(env_)
		, outputStreamClass(env_)
		, stringClass(env_)
		, uriClass(env_, stringClass)
		, cls(env_, JavaRef(env_->GetObjectClass(instance_)))
		// We need to keep it global because we are going to use it in multiple threads
		, instance(env_->NewGlobalRef(instance_), true)
		, m_getclass_mutex(System::CriticalSection::Create(true)) // TODO: [Florian] what arguments to pass?
	{}

	ExceptionClass exceptionClass;
	InputStreamClass inputStreamClass;
	OutputStreamClass outputStreamClass;
	ReflexActivityClass cls;
	StringClass stringClass;
	UriClass uriClass;
	JavaRef<jobject> instance;

	// Only breaks if the debugger is active (from Android Studio)
	void BreakDebugger() const;

	template<class CLASS>
	CLASS& GetClass(JNIEnv* env) {
		System::CriticalSection::Lock lock(m_getclass_mutex);
		Key32 name = CLASS::key;
		auto found = additionalClassMappings.Search(name);
		if (found) return *Cast<CLASS>(found->Adr());

		TRef<CLASS> created = REFLEX_CREATE(CLASS, env);
		additionalClassMappings.Set(name, created);
		return *created;
	}

	// We pass the JNIEnv each time because these might be called from multiple threads
	CString GetApplicationDir(JNIEnv* env) {
		JavaRef retVal(cls.getApplicationDir(env, instance));
		return retVal.ToNative<CString>(env);
	}

	CString GetUserdataDir(JNIEnv* env) {
		JavaRef retVal(cls.getUserdataDir(env, instance));
		return retVal.ToNative<CString>(env);
	}

	CString GetUserdocsDir(JNIEnv* env) {
		JavaRef retVal(cls.getUserdocsDir(env, instance));
		return retVal.ToNative<CString>(env);
	}

	CString GetTempDir(JNIEnv* env) {
		JavaRef retVal(cls.getTempDir(env, instance));
		return retVal.ToNative<CString>(env);
	}

	// This method will wait and call Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onRecordingPermissionResult when finished. Slow operation and requires to attach Java to the thread, so use HasAlreadyGotRecordingPermission() before.
	bool RequestRecordingPermission(JNIEnv* env);
	bool HasAlreadyGotRecordingPermission() const { return m_got_recording_permission; }
	Array<Reference<AudioDeviceInstance>> GetAudioDevices(JNIEnv* env, const WString::View& defaultDeviceName);

	void ShowMessageBox(JNIEnv* env, const WString::View& title, const WString::View& text, Function<void(UInt32 clickedButton)>&& callback) const;
	void ShowFilePicker(JNIEnv* env, const ArrayView<WString>& mime_types, ExternalResourceRef::AccessMode accessType, bool allowMultiple, const WString::View& suggestedFileName, Function<void(const Array<Array<UInt8>>& uris)>&& callback) const;

	void OpenInputStream(JNIEnv* env, const JavaRef<jobject>& fileUri, JavaRef<jobject>& result) const;
	Int64 GetFileSize(JNIEnv* env, const JavaRef<jobject>& fileUri) const;
	void ReadFileFully(JNIEnv *env, const JavaRef<jobject>& fileUri, Array<UInt8>& dest) const;
	void WriteFileFully(JNIEnv *env, const JavaRef<jobject>& fileUri, const Array<UInt8>& data) const;

	void CopyTextToClipboard(JNIEnv* env, const WString::View& text) const;
	WString GetTextFromClipboard(JNIEnv* env) const;

	int GetNumProcessors(JNIEnv* env) const;

	CString GetAndroidId(JNIEnv* env) const;

	void EnableBackgroundAudioService(JNIEnv* env, bool enabled) const;
	void FinishActivity(JNIEnv* env) const;

	void ShowKeyboard(JNIEnv* env, bool shown) const;
	void EnableTextInput(JNIEnv* env, bool enabled, VirtualKeyboardInputType type, const WString::View& buffer, Pair<UInt> selection) const;

	void SetRequestedOrientation(JNIEnv* env, int orientation) const;

	CString GetLanguage(JNIEnv* env) const;
	bool IsDarkTheme(JNIEnv* env) const;
	float GetFontSize(JNIEnv* env) const;

private:
	Map<Key32, Reference<Object>> additionalClassMappings;
	bool m_got_recording_permission = false;
	bool m_showkeyboard_cache = false;
	Reference <System::CriticalSection> m_getclass_mutex;
};

extern Reference<ReflexActivityInstance, false> g_reflexActivityInstance;

template<bool THROWS_EXCEPTIONS>
const InputStreamClass &InputStreamInstance<THROWS_EXCEPTIONS>::cls() {
	return g_reflexActivityInstance->inputStreamClass;
}

template<bool THROWS_EXCEPTIONS>
void InputStreamInstance<THROWS_EXCEPTIONS>::Reopen(const JavaRef<jobject>& fileUri) {
	if (instance) {
		Close();
	}
	g_reflexActivityInstance->OpenInputStream(env, fileUri, instance);
	if (!instance.IsGlobalRef()) {
		instance.MakeGlobalRef(env);
	}
}

template<bool THROWS_EXCEPTIONS>
const OutputStreamClass &OutputStreamInstance<THROWS_EXCEPTIONS>::cls() {
	return g_reflexActivityInstance->outputStreamClass;
}

REFLEX_END
