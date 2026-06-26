#include "[require].h"
#include "../../common/debug.h"
#include "../../common/utf8.h"
#include "../android_utils.hpp"

REFLEX_NS(Reflex::System::Android::Jni)

static thread_local UInt g_threadEnvCount = 0;
static thread_local JNIEnv* g_threadEnv = nullptr;

JavaVM *g_javaVm = nullptr;

//
// AttachedJavaEnv
// MEMO: this object should never throw an exception or fail in any way,
// since it's used in debugging situations where Java might not be set up properly.
//
AttachedJavaEnv::AttachedJavaEnv() {
	if (g_threadEnv) {
		g_threadEnvCount += 1;
	}
	else if (g_javaVm) {
		jint status = g_javaVm->GetEnv((void**)&g_threadEnv, JNI_VERSION_1_6);
		if (status == JNI_EDETACHED) {
			JavaVMAttachArgs args = {
				.version = JNI_VERSION_1_6,
				.name = nullptr,
				.group = nullptr
			};
			//DEV_LOG("Attaching JVM to thread", GetThreadID()));
			status = g_javaVm->AttachCurrentThread(&g_threadEnv, &args);
			g_threadEnvCount = status == JNI_OK ? 1 : 0;
		}
		else {
			// Was already attached, make sure that we won't free it
			g_threadEnvCount = 2;
		}
	}
	else {
		DEV_ERROR(g_javaVm, "*** Trying to call Java while the VM is not ready yet ***");
	}
}

AttachedJavaEnv::~AttachedJavaEnv() {
	// g_javaVm might have been made null if we called deinitJava (as we should) before C++ destroys the remaining global objects
	if (--g_threadEnvCount == 0 && g_javaVm) {
		//DEV_LOG("Detaching JVM from thread", GetThreadID());
		g_javaVm->DetachCurrentThread();
		g_threadEnv = nullptr;
	}
}

JNIEnv *AttachedJavaEnv::get() const { return g_threadEnv; }


//
// JavaExceptionCheckScope
//
void JavaExceptionCheckScope::AssertOk() {
	if constexpr (!REFLEX_DEBUG) return;

	if (m_env->ExceptionCheck()) {
		JavaRef e (m_env->ExceptionOccurred());
		m_env->ExceptionClear();
		g_reflexActivityInstance->exceptionClass.PrintStackTrace(m_env, e);
		throw std::runtime_error(g_reflexActivityInstance->exceptionClass.GetMessage(m_env, e).GetData());
	}
}

bool JavaExceptionCheckScope::CheckAndClearException() {
	if (m_env->ExceptionCheck()) {
		if constexpr (REFLEX_DEBUG) {
			JavaRef e(m_env->ExceptionOccurred());
			m_env->ExceptionClear();
			g_reflexActivityInstance->exceptionClass.PrintStackTrace(m_env, e);
			return true;
		}

		m_env->ExceptionClear();
		return true;
	}

	return false;
}

void JavaObjectToNative(JNIEnv *env, jstring obj, CString &out_result) {
	REFLEX_ASSERT(g_reflexActivityInstance)

	if (!obj) {
		out_result = {};
		return;
	}

	g_reflexActivityInstance->stringClass.GetBytes(env, obj, out_result);
}

void JavaFromNative(JNIEnv *env, const Array<WChar>::View &source, JavaRef<jstring> &out_dest) {
	auto utf_str = Common::ToUTF8(source);

	out_dest.SetAsOwned(env->NewStringUTF(utf_str.GetData()));
}

void JavaObjectToNative(JNIEnv *env, jstring obj, WString &out_result) {
	if (!obj) {
		out_result = {};
		return;
	}

	auto numChars = env->GetStringUTFLength(jstring(obj));
	auto pChars = env->GetStringUTFChars(jstring(obj), nullptr);
	out_result = Common::DecodeUTF8({ Reinterpret<UInt8>(pChars), UInt(numChars) });
	env->ReleaseStringUTFChars(jstring(obj), pChars);
}

void JavaObjectToNative(JNIEnv *env, jobject obj, Array<Int> &out_result) {
	if (!obj) {
		out_result.SetSize(0);
		return;
	}

	auto javaArray = (jintArray)obj;
	auto len = env->GetArrayLength(javaArray);
	out_result.SetSize(len);
	env->GetIntArrayRegion(javaArray, 0, len, out_result.GetData());
}

void JavaObjectToNative(JNIEnv *env, jobject obj, Array<Float> &out_result) {
	if (!obj) {
		out_result.SetSize(0);
		return;
	}

	auto javaArray = (jfloatArray)obj;
	auto len = env->GetArrayLength(javaArray);
	out_result.SetSize(len);
	env->GetFloatArrayRegion(javaArray, 0, len, out_result.GetData());
}

void JavaObjectToNative(JNIEnv *env, jobject obj, Array<UInt8> &out_result) {
	if (!obj) {
		out_result.SetSize(0);
		return;
	}

	auto javaArray = (jbyteArray)obj;
	auto len = env->GetArrayLength(javaArray);
	out_result.SetSize(len);
	env->GetByteArrayRegion(javaArray, 0, len, Reinterpret<jbyte>(out_result.GetData()));
}

void JavaObjectToNative(JNIEnv *env, jobject obj, Array<Pair<CString>> &out_result) {
	if (!obj) {
		out_result.Clear();
		return;
	}

	auto arraylistClass = env->GetObjectClass(obj);
	auto sizeMethod = env->GetMethodID(arraylistClass, "size", MethodSignature<int>());
	auto getItemMethod = env->GetMethodID(arraylistClass, "get", MethodSignature<Object, int>());

	out_result.SetSize(env->CallIntMethod(obj, sizeMethod) / 2);

	for (int i = 0; i < out_result.GetSize(); i++) {
		JavaRef first { env->CallObjectMethod(obj, getItemMethod, i * 2) };
		JavaRef second { env->CallObjectMethod(obj, getItemMethod, i * 2 + 1) };

		JavaObjectToNative(env, jstring(first.ref), out_result[i].a);
		JavaObjectToNative(env, jstring(second.ref), out_result[i].b);
	}
}

void JavaFromNative(JNIEnv *env, const Reflex::Array<char>::View &source, JavaRef<jstring> &out_dest) {
	out_dest.SetAsOwned(env->NewStringUTF(source.data));
}

void JavaFromNative(JNIEnv *env, const char *source, JavaRef<jstring> &out_dest) {
	out_dest.SetAsOwned(env->NewStringUTF(source));
}

void JavaFromNative(JNIEnv *env, const Reflex::ArrayView<Reflex::UInt8> &bytes, JavaRef<jobject> &out_dest) {
	jbyteArray arr = env->NewByteArray(bytes.size);
	env->SetByteArrayRegion(arr, 0, bytes.size, (const jbyte*)bytes.data);
	out_dest.SetAsOwned(arr);
}

//	void JavaObjectToNative(JNIEnv* env, jobject obj, Map<CString, CString>& out_result) {
//		out_result.Clear();
//
//		if (!obj) return;
//
//		jclass mapClass = env->GetObjectClass(obj);
//		jmethodID entrySet = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
//		jobject set = env->CallObjectMethod(obj, entrySet);
//		// Obtain an iterator over the Set
//		JavaRef<jclass> setClass(env->FindClass("java/util/Set"));
//		jmethodID iterator = env->GetMethodID(setClass.ref, "iterator", "()Ljava/util/Iterator;");
//		jobject iter = env->CallObjectMethod(set, iterator);
//		// Get the Iterator method IDs
//		JavaRef<jclass> iteratorClass(env->FindClass("java/util/Iterator"));
//		jmethodID hasNext = env->GetMethodID(iteratorClass.ref, "hasNext", "()Z");
//		jmethodID next = env->GetMethodID(iteratorClass.ref, "next", "()Ljava/lang/Object;");
//		// Get the Entry class method IDs
//		JavaRef<jclass> entryClass(env->FindClass("java/util/Map$Entry"));
//		jmethodID getKey = env->GetMethodID(entryClass.ref, "getKey", "()Ljava/lang/Object;");
//		jmethodID getValue = env->GetMethodID(entryClass.ref, "getValue", "()Ljava/lang/Object;");
//
//		// Iterate over the entry Set
//		while (env->CallBooleanMethod(iter, hasNext)) {
//			jobject entry = env->CallObjectMethod(iter, next);
//			jstring key = (jstring)env->CallObjectMethod(entry, getKey);
//			jstring value = (jstring)env->CallObjectMethod(entry, getValue);
//			const char* keyStr = env->GetStringUTFChars(key, nullptr);
//			if (keyStr) {
//				const char *valueStr = env->GetStringUTFChars(value, nullptr);
//				out_result[keyStr] = valueStr;
//
//				env->DeleteLocalRef(entry);
//				env->ReleaseStringUTFChars(key, keyStr);
//				env->DeleteLocalRef(key);
//				env->ReleaseStringUTFChars(value, valueStr);
//			}
//			env->DeleteLocalRef(value);
//		}
//	}

REFLEX_END
