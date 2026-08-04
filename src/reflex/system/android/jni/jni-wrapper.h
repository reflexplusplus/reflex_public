#include "../../common/debug.h"

namespace Reflex::System::Android::Jni {

	extern JavaVM *g_javaVm;

	// To be called from the java thread
	void initJava(JNIEnv *env, jobject mainActivity);

	// To be called from the java thread
	void deinitJava();

	// Warning: needs cleaning (env->DeleteLocalRef, or assign to a JavaRef). Only use for user defined classes, not system classes!
	jclass findClass(JNIEnv* env, const char* name);

	// Use it as a block variable to ensure that the current thread is attached to the JVM, and detached at the end.
// If the thread is already acquired, does nothing (it will be detached when the previous AttachedJavaEnv which has acquired it, gets out of scope).
// Declare an object of this type to ensure you have an active environment in the current scope.
	struct AttachedJavaEnv {
		AttachedJavaEnv();
		~AttachedJavaEnv();

		AttachedJavaEnv(const AttachedJavaEnv&) = delete;
		AttachedJavaEnv& operator = (const AttachedJavaEnv&) = delete;

		JNIEnv* get() const;
		inline JNIEnv* operator->() const { return get(); }
		inline operator JNIEnv* () const { return get(); }
	};

	// Managed reference to a Java object, with automatic release once it gets deleted.
	// For objects that are local (current function) only.
	// If they need to be maintained, or they will be used by multiple threads, call MakeGlobalRef.
	// In Java, every call that returns an object reference (CallObjectMethod, FindClass, GetObjectClass, NewObject, NewByteArray, GetStringUTFChars, …) returns a local reference that needs to be released. That's why, instead of manipulating jobject instances directly, you can construct them in a JavaRef, and they'll be released when the JavaRef goes out of scope.
	// Do not use a JavaRef to reference a jobject that does not "belong" to you (which can happen if the reference has been passed to you as a parameter to a JNI call from Java to C++, since you don't need to release those). If you need to retain them, just construct a global reference, using JavaRef(env, env->NewGlobalRef(jobject), /* isGlobalRef = /* true); this will not touch the local reference counter, but will create a global one, retained by the JavaRef and deleted with it at the end.
	// If the object is alreaday global, you need to pass true to isGlobalRef_ so that the appropriate deletion is called.
	// You cannot pass true to isGlobalRef_ and pass a local reference; if you want to "make" a global reference (takes either a local or a global ref, and increments the global ref count), use GlobalJavaRef below.
	template<class RefType = jobject>
	struct JavaRef {
		explicit JavaRef() : JavaRef(nullptr) {}

		explicit JavaRef(RefType ref_, bool isGlobalRef_ = false)
			: ref(ref_)
			, isGlobalRef(isGlobalRef_)
		{}

		template<class T>
		explicit JavaRef(JNIEnv* env_, T constructFrom)
			: JavaRef()
		{
			JavaFromNative(env_, constructFrom, *this);
		}

		// There is no ref counting, so once you have a owner, it has to remain so, or you need to use other semantics such as DetachOwnership or MakeGlobalRef.
		JavaRef(const JavaRef&) = delete;

		JavaRef& operator = (const JavaRef&) = delete;

		JavaRef(JavaRef&& old) noexcept
			: ref(old.ref)
		{
			old.ref = nullptr;
		}

		~JavaRef() {
			Release();
		}

		RefType DetachOwnership() {
			RefType ref_ = ref;
			ref = nullptr;
			return ref_;
		}

		// Creates a global ref (owned), and delete the current local one
		void MakeGlobalRef(JNIEnv* env) {
			if (!ref) return;
			REFLEX_ASSERT(!isGlobalRef)
			SetAsOwned((RefType) env->NewGlobalRef(ref), true);
		}

		// Makes a local reference as owned (meaning that it will be automatically released when this JavaRef goes out of scope)
		void SetAsOwned(RefType ref_, bool isGlobalRef_ = false) {
			Release();
			this->ref = ref_;
			this->isGlobalRef = isGlobalRef_;
		}

		// For convenience; avoid using for performance
		template<class NativeType>
		NativeType ToNative(JNIEnv* env) const {
			void JavaObjectToNative(JNIEnv*, RefType obj, NativeType& out);
			NativeType result;
			JavaObjectToNative(env, ref, result);
			return result;
		}

		bool IsGlobalRef() const { return isGlobalRef; }
		operator RefType() const { return ref; }

		RefType ref = nullptr;

	private:
		bool isGlobalRef = false;

		void Release() {
			if (ref) {
				// Release can occur on another thread than the one on which the object was created.
				AttachedJavaEnv env;
				if (isGlobalRef) env->DeleteGlobalRef(ref);
				else env->DeleteLocalRef(ref);
				ref = nullptr;
			}
		}
	};

	JavaRef(JNIEnv*, const WString::View&) -> JavaRef<jstring>;
	JavaRef(JNIEnv*, const CString::View&) -> JavaRef<jstring>;
	JavaRef(JNIEnv*, const WString&) -> JavaRef<jstring>;
	JavaRef(JNIEnv*, const CString&) -> JavaRef<jstring>;
	JavaRef(JNIEnv*, const char*) -> JavaRef<jstring>;

	template<class RefType = jobject>
	struct GlobalJavaRef : JavaRef<RefType> {
		GlobalJavaRef(JNIEnv* env, RefType ref_)
			: JavaRef<RefType>((RefType)env->NewGlobalRef(ref_), true)
		{}

		GlobalJavaRef(JNIEnv* env, const GlobalJavaRef<RefType> & to_copy)
			: JavaRef<RefType>((RefType) env->NewGlobalRef(to_copy.ref), true)
		{}
	};

	struct [[maybe_unused]] Nothing {};

	template<class T> const char* JavaSignature = "Lundefined/Signature";
	template<> inline const char* JavaSignature<Nothing> = "";
	template<> inline const char* JavaSignature<void> = "V";
	template<> inline const char* JavaSignature<bool> = "Z";
	template<> inline const char* JavaSignature<Float> = "F";
	template<> inline const char* JavaSignature<int> = "I";
	template<> inline const char* JavaSignature<Int64> = "J";
	template<> inline const char* JavaSignature<CString> = "Ljava/lang/String;";
	template<> inline const char* JavaSignature<const char*> = "Ljava/lang/String;";
	template<> inline const char* JavaSignature<void*> = "Ljava/nio/ByteBuffer;";
	template<> inline const char* JavaSignature<Array<UInt8>> = "[B";
	template<> inline const char* JavaSignature<Array<Int>> = "[I";
	template<> inline const char* JavaSignature<Array<Float>> = "[F";
	template<> inline const char* JavaSignature<Array<CString>> = "[Ljava/lang/String;";
	template<> inline const char* JavaSignature<Map<CString, CString>> = "Ljava/util/Map;";
	// This actually gets returned as a list of String (not pair), with two entries per pair
	template<> inline const char* JavaSignature<Array<Pair<CString>>> = "Ljava/util/ArrayList;";
	template<> inline const char* JavaSignature<Object> = "Ljava/lang/Object;";

	template<class ReturnType, class Arg1 = Nothing, class Arg2 = Nothing, class Arg3 = Nothing, class Arg4 = Nothing, class Arg5 = Nothing, class Arg6 = Nothing>
	struct MethodSignature {
		MethodSignature()
			: str(Join("(", JavaSignature<Arg1>, JavaSignature<Arg2>, JavaSignature<Arg3>, JavaSignature<Arg4>, JavaSignature<Arg5>, JavaSignature<Arg6>, ")", JavaSignature<ReturnType>))
		{}

		operator const char* () const { return str.GetData(); }

	private:
		CString str;
	};


	//
	// Use as a scoped variable around JNI calls, to prevent exceptions happening in the Java
	// code from going unnoticed.
	//
	struct JavaExceptionCheckScope {
		explicit JavaExceptionCheckScope(JNIEnv* env)
			: m_env(env)
		{}

		~JavaExceptionCheckScope() {
			AssertOk();
		}

		// Throws the exception generated by the last call as a C++ exception of type CString.
		// If no exception happened, does nothing.
		// Note: does nothing in release mode.
		void AssertOk();

		// Use this after a call where a Java exception could be "normal" (expected).
		// Returns whether there was an exception. If false, the previous JNI call went properly.
		// In debug mode, prints the exception message to the console.
		bool CheckAndClearException();

	private:
		JNIEnv* m_env;
	};

	static void AssertNothingThrown(JNIEnv* env) {
		JavaExceptionCheckScope scope(env);
	}


	//
	// Method wrappers
	//
	template<typename T> struct ReflexTypeToJava;

	template<> struct ReflexTypeToJava<void> { using type = void; };

	template<> struct ReflexTypeToJava<int> { using type = jint; };

	template<> struct ReflexTypeToJava<float> { using type = jfloat; };

	template<> struct ReflexTypeToJava<double> { using type = jdouble; };

	template<> struct ReflexTypeToJava<bool> { using type = jboolean; };

	template<> struct ReflexTypeToJava<Int64> { using type = jlong; };

	template<> struct ReflexTypeToJava<CString> { using type = jstring; };

	template<> struct ReflexTypeToJava<const char*> { using type = jstring; };

	template<> struct ReflexTypeToJava<Object> { using type = jobject; };

	template<> struct ReflexTypeToJava<Array<Pair<CString>>> { using type = jobject; };

	template<> struct ReflexTypeToJava<Array<UInt8>> { using type = jstring; };

	template<typename T>
	using ReflexTypeToJava_t = typename ReflexTypeToJava<T>::type;


	template <class R, class ... VARGS>
	R PerformJavaCall(JNIEnv* env, jobject instance, jmethodID method, VARGS &&... v) {
		using std::is_same_v;

		REFLEX_ASSERT(instance && method)

		if constexpr (is_same_v<R, void>)			return env->CallVoidMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jint>)		return env->CallIntMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jlong>)		return env->CallLongMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jfloat>)	return env->CallFloatMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jdouble>)	return env->CallDoubleMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jboolean>)	return env->CallBooleanMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jobject>)	return env->CallObjectMethod(instance, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jstring>) {
			return (jstring)env->CallObjectMethod(instance, method, std::forward<VARGS>(v)...);
		}
		else if constexpr (is_same_v<R, jobjectArray>) {
			return (jobjectArray)env->CallObjectMethod(instance, method, std::forward<VARGS>(v)...);
		}
		else {
			static_assert(always_false_v<R>, "Unsupported return type");
		}
	}


	template <class R, class ... VARGS>
	R PerformStaticJavaCall(JNIEnv* env, jclass cls, jmethodID method, VARGS &&... v) {
		using std::is_same_v;

		REFLEX_ASSERT(cls && method)

		if constexpr (is_same_v<R, void>)			return env->CallStaticVoidMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jint>)		return env->CallStaticIntMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jlong>)		return env->CallStaticLongMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jfloat>)	return env->CallStaticFloatMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jdouble>)	return env->CallStaticDoubleMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jboolean>)	return env->CallStaticBooleanMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jobject>)	return env->CallStaticObjectMethod(cls, method, std::forward<VARGS>(v)...);
		else if constexpr (is_same_v<R, jstring>) {
			return (jstring)env->CallStaticObjectMethod(cls, method, std::forward<VARGS>(v)...);
		}
		else if constexpr (is_same_v<R, jobjectArray>) {
			return (jobjectArray)env->CallStaticObjectMethod(cls, method, std::forward<VARGS>(v)...);
		}
		else {
			static_assert(always_false_v<R>, "Unsupported return type");
		}
	}


	template <class ReturnType, class Arg1 = Nothing, class Arg2 = Nothing, class Arg3 = Nothing, class Arg4 = Nothing, class Arg5 = Nothing, class Arg6 = Nothing>
	struct JavaMethod {

		JavaMethod(JNIEnv* env, JavaRef<jclass>& cls, const char* method_name)
			: method(env->GetMethodID(cls.ref, method_name, MethodSignature<ReturnType, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>()))
		{}

		template <class ... VARGS>
		auto CallWithScope(JNIEnv* env, jobject instance, const JavaExceptionCheckScope& scope, VARGS &&... v) const {
			if constexpr (std::is_same_v<ReturnType, void>) {
				PerformJavaCall<ReturnType>(env, instance, method, std::forward<VARGS>(v)...);
			}
			else {
				return PerformJavaCall<ReflexTypeToJava_t<ReturnType>>(env, instance, method, std::forward<VARGS>(v)...);
			}
		}

		template <class ... VARGS>
		auto Call(JNIEnv* env, jobject instance, VARGS &&... v) const {
			return CallWithScope(env, instance, JavaExceptionCheckScope(env), std::forward<VARGS>(v)...);
		}

		template <class ... VARGS>
		auto operator () (JNIEnv* env, jobject instance, VARGS &&... v) const {
			return CallWithScope(env, instance, JavaExceptionCheckScope(env), std::forward<VARGS>(v)...);
		}

	private:
		jmethodID method;
	};


	template <class Arg1 = Nothing, class Arg2 = Nothing, class Arg3 = Nothing, class Arg4 = Nothing, class Arg5 = Nothing, class Arg6 = Nothing>
	struct JavaConstructor {

		JavaConstructor(JNIEnv* env, JavaRef<jclass>& cls_)
			: cls(cls_)
			, method(env->GetMethodID(cls_.ref, "<init>", MethodSignature<void, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>()))
		{}

		template <class ... VARGS>
		void MakeNewInstance(JNIEnv* env, JavaRef<jobject>& out_ownedinstance, JavaExceptionCheckScope& scope, VARGS &&... v) const {
			REFLEX_ASSERT(method)

			out_ownedinstance.SetAsOwned(env->NewObject(cls.ref, method, std::forward<VARGS>(v)...), false);
			out_ownedinstance.MakeGlobalRef(env);
		}

		template <class ... VARGS>
		void operator () (JNIEnv* env, JavaRef<jobject>& out_ownedinstance, VARGS &&... v) const {
			JavaExceptionCheckScope scope(env);
			MakeNewInstance(env, out_ownedinstance, std::forward<VARGS>(v)...);
		}

	private:
		JavaRef<jclass>& cls;
		jmethodID method;
	};

	template <class ReturnType, class Arg1 = Nothing, class Arg2 = Nothing, class Arg3 = Nothing, class Arg4 = Nothing, class Arg5 = Nothing, class Arg6 = Nothing>
	struct JavaStaticMethod {

		JavaStaticMethod(JNIEnv* env, JavaRef<jclass>& cls_, const char* method_name)
			: cls(cls_)
			, method(env->GetStaticMethodID(cls_.ref, method_name, MethodSignature<ReturnType, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>()))
		{}

		template <class ... VARGS>
		auto CallWithScope(JNIEnv* env, const JavaExceptionCheckScope& scope, VARGS &&... v) const {
			if constexpr (std::is_same_v<ReturnType, void>) {
				PerformStaticJavaCall<ReturnType>(env, method, std::forward<VARGS>(v)...);
			}
			else {
				return PerformStaticJavaCall<ReflexTypeToJava_t<ReturnType>>(env, cls, method, std::forward<VARGS>(v)...);
			}
		}

		template <class ... VARGS>
		auto Call(JNIEnv* env, VARGS &&... v) const {
			return CallWithScope(env, JavaExceptionCheckScope(env), std::forward<VARGS>(v)...);
		}

		template <class ... VARGS>
		auto operator () (JNIEnv* env, VARGS &&... v) const {
			return CallWithScope(env, JavaExceptionCheckScope(env), std::forward<VARGS>(v)...);
		}

	private:
		JavaRef <jclass>& cls;
		jmethodID method;
	};


/**
 * Conversions from java objects to native.
 */
	inline void JavaFromNative(JNIEnv* env, const bool source, jboolean& out_dest) { out_dest = source; }

	inline void JavaFromNative(JNIEnv* env, const int source, jint& out_dest) { out_dest = source; }

	void JavaFromNative(JNIEnv* env, const CString::View& source, JavaRef<jstring>& out_dest);

	void JavaFromNative(JNIEnv* env, const WString::View& source, JavaRef<jstring>& out_dest);

	void JavaFromNative(JNIEnv* env, const char* source, JavaRef<jstring>& out_dest);

	void JavaFromNative(JNIEnv* env, const ArrayView<UInt8>& bytes, JavaRef<jobject>& out_dest);

	template<class RefType, class T>
	void JavaFromNativeArray(JNIEnv* env, jclass arrayElementClass, const ArrayView<T>& source, JavaRef<jobject>& out_dest) {
		JavaExceptionCheckScope scope(env);
		JavaRef<RefType> item;
		jobjectArray arr;

		arr = env->NewObjectArray(jint(source.size), arrayElementClass, nullptr);
		scope.AssertOk();

		REFLEX_LOOP(idx, source.size) {
			JavaFromNative(env, source[idx], item);
			env->SetObjectArrayElement(arr, idx, item);
			scope.AssertOk();
		}

		out_dest.SetAsOwned(arr);
	}

	void JavaObjectToNative(JNIEnv* env, jstring obj, CString& out_result);

	void JavaObjectToNative(JNIEnv* env, jstring obj, WString& out_result);

	void JavaObjectToNative(JNIEnv* env, jobject obj, Array<Int>& out_result);

	void JavaObjectToNative(JNIEnv* env, jobject obj, Array<Float>& out_result);

	void JavaObjectToNative(JNIEnv* env, jobject obj, Array<UInt8>& out_result);

	void JavaObjectToNative(JNIEnv* env, jobject obj, Array<Pair<CString>>& out_result);

	template<class T>
	void JavaObjectToNativeArray(JNIEnv* env, jobjectArray array, Array<T>& out_result) {
		auto len = array ? env->GetArrayLength(array) : 0;
		out_result.SetSize(len);

		REFLEX_LOOP(i, len) {
			JavaRef el(env->GetObjectArrayElement(array, i));
			JavaObjectToNative(env, el, out_result[i]);
		}
	}
}
