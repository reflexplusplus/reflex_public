#include "java-classes.h"
#include "../android_utils.hpp"

REFLEX_NS(Reflex::System::Android::Jni)

Reference<ReflexActivityInstance, false> g_reflexActivityInstance;

static JavaRef<jobject> g_classLoader;
static jmethodID g_findClassMethod;


//
// Static functions
//
void initJava(JNIEnv *env, jobject mainActivity) {
	if (!g_javaVm) {
		env->GetJavaVM(&g_javaVm);

		// https://stackoverflow.com/questions/13263340/findclass-from-any-thread-in-android-jni%C2%A0
		// Note that "randomClass" has to be one of the classes of our SDK, or it might pick another class loader
		JavaRef classClass(env->GetObjectClass(mainActivity));
		JavaRef classLoaderClass(env->FindClass("java/lang/ClassLoader"));
		auto getClassLoaderMethod = env->GetMethodID(classClass.ref, "getClassLoader",
													 "()Ljava/lang/ClassLoader;");
		JavaRef classLoader((jclass) env->CallObjectMethod(mainActivity, getClassLoaderMethod));
		g_classLoader.SetAsOwned(env->NewGlobalRef(classLoader), true);
		g_findClassMethod = env->GetMethodID(classLoaderClass.ref, "findClass",
											 "(Ljava/lang/String;)Ljava/lang/Class;");
	}

// Creates and manages a global ref to the object passed
	g_reflexActivityInstance = REFLEX_CREATE(ReflexActivityInstance, env, mainActivity);
}

void deinitJava() {
	g_classLoader.SetAsOwned(nullptr);
	g_reflexActivityInstance.Clear();
	g_javaVm = nullptr;
}

jclass findClass(JNIEnv* env, const char* name) {
	return static_cast<jclass>(env->CallObjectMethod(g_classLoader, g_findClassMethod, JavaRef(env, name).ref));
}

UriClass::UriClass(JNIEnv *env, StringClass &stringClass_)
	: cls(env->FindClass("android/net/Uri"))
	, stringClass(stringClass_)
	, parse(env->GetStaticMethodID(cls.ref, "parse", MethodSignature<UriClass, CString>()))
{
	cls.MakeGlobalRef(env);
}

void ReflexActivityInstance::BreakDebugger() const {
#if (REFLEX_DEBUG)
	AttachedJavaEnv env;
	if (env.get() != nullptr && cls.isDebuggerAttached(env, instance)) {
		// https://stackoverflow.com/questions/173618/is-there-a-portable-equivalent-to-debugbreak-debugbreak
		// You can resume the execution by entering `continue` on the LLDB or GDB console tab
		__builtin_trap();
	}
#endif
}

bool ReflexActivityClass::IsBackgroundAudioServiceRunning(JNIEnv* env) const {
	JavaExceptionCheckScope scope(env);

	return isBackgroundAudioServiceRunning(env);
}

CString ReflexActivityClass::GetOperatingSystemVersion(JNIEnv* env) const {
	return JavaRef(getOperatingSystemVersion(env)).ToNative<CString>(env);
}

CString ReflexActivityInstance::GetAndroidId(JNIEnv *env) const {
	return JavaRef(cls.getAndroidId(env, instance)).ToNative<CString>(env);
}

void ReflexActivityInstance::OpenInputStream(JNIEnv* env, const JavaRef<jobject>& fileUri, JavaRef<jobject>& result) const {
	result.SetAsOwned(cls.openInputStream(env, instance, fileUri.ref));
}

Int64 ReflexActivityInstance::GetFileSize(JNIEnv* env, const JavaRef<jobject>& fileUri) const {
	return cls.getFileSize(env, instance, fileUri.ref);
}

void ReflexActivityInstance::ReadFileFully(JNIEnv *env, const JavaRef<jobject>& fileUri, Array<UInt8>& dest) const {
	auto size = GetFileSize(env, fileUri);
	if (size <= 0) {
		dest.SetSize(0);
		return;
	}

	dest.SetSize(size);

	JavaRef buf (env->NewDirectByteBuffer(dest.GetData(), size));
	cls.readFileFully(env, instance, fileUri.ref, buf.ref);
}

void ReflexActivityInstance::WriteFileFully(JNIEnv *env, const JavaRef<jobject>& fileUri, const Array<UInt8>& data) const {
	JavaRef buf (env->NewDirectByteBuffer((void*)data.GetData(), data.GetSize()));

	cls.writeFileFully(env, instance, fileUri.ref, buf.ref);
}

void ReflexActivityInstance::CopyTextToClipboard(JNIEnv* env, const WString::View& text) const {
	cls.copyTextToClipboard(env, instance, JavaRef(env, text).ref);
}

WString ReflexActivityInstance::GetTextFromClipboard(JNIEnv* env) const {
	return JavaRef(cls.getTextFromClipboard(env, instance)).ToNative<WString>(env);
}

int ReflexActivityInstance::GetNumProcessors(JNIEnv *env) const {
	JavaExceptionCheckScope scope(env);
	int result = cls.getNumProcessors.CallWithScope(env, instance, scope);

	return scope.CheckAndClearException() ? 1 : result;
}

void ReflexActivityInstance::EnableBackgroundAudioService(JNIEnv* env, bool enabled) const {
	cls.enableBackgroundAudioService(env, instance, enabled);
}

void ReflexActivityInstance::FinishActivity(JNIEnv* env) const {
	cls.finishActivity(env, instance);
}

void ReflexActivityInstance::ShowKeyboard(JNIEnv* env, bool shown) const {
	cls.showKeyboard(env, instance, shown);
}

void ReflexActivityInstance::EnableTextInput(JNIEnv* env, bool enabled, VirtualKeyboardInputType type, const WString::View& buffer, Pair<UInt> selection) const {
	cls.enableTextInput(env, instance, enabled, type, JavaRef(env, buffer).ref, selection.a, selection.a + selection.b);
}

void ReflexActivityInstance::SetRequestedOrientation(JNIEnv *env, int orientation) const {
	cls.setRequestedOrientation(env, instance, orientation);
}

CString ReflexActivityInstance::GetLanguage(JNIEnv* env) const {
	JavaExceptionCheckScope scope(env);
	JavaRef result { cls.getLanguage.CallWithScope(env, instance, scope) };

	if (scope.CheckAndClearException()) return "en";

	return result.ToNative<CString>(env);
}

bool ReflexActivityInstance::IsDarkTheme(JNIEnv* env) const {
	return cls.isDarkTheme(env, instance);
}

float ReflexActivityInstance::GetFontSize(JNIEnv* env) const {
	return cls.getFontSize(env, instance);
}

REFLEX_END
