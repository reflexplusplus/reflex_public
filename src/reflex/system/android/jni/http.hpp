REFLEX_BEGIN_INTERNAL(Reflex::System::Android::Jni)

struct HttpConnectionClass : Object {
	explicit HttpConnectionClass(JNIEnv* env)
		: cls(env, findClass(env, "com/reflex_multimedia/reflex/sdk/HttpConnection"))
		, Constructor(env, cls)
		, PerformRequest(env, cls, "PerformRequest")
		, SetBody(env, cls, "SetBody")
		, SetMethod(env, cls, "SetMethod")
		, PutHeader(env, cls, "PutHeader")
		, SetTimeout(env, cls, "SetTimeout")
		, ReadResponseBody(env, cls, "ReadResponseBody")
		, ReadResponseHeaders(env, cls, "ReadResponseHeaders")
		, Dispose(env, cls, "Dispose")
	{}

	static constexpr Key32 key = "HttpConnectionClass";

	GlobalJavaRef <jclass> cls;
	JavaConstructor <const char*> Constructor;
	JavaMethod <int> PerformRequest;
	JavaMethod <void, Array<UInt8>> SetBody;
	JavaMethod <void, const char*> SetMethod;
	JavaMethod <void, const char*, const char*> PutHeader;
	JavaMethod <void, Float, Float> SetTimeout;
	JavaMethod <Array<UInt8>, int> ReadResponseBody;
	JavaMethod <Array<Pair<CString>>> ReadResponseHeaders;
	JavaMethod <void> Dispose;
};

// ⚠️ Can only be used on one thread at once (HttpConnectionClass not)
struct HttpConnectionInstance {
	HttpConnectionInstance(JNIEnv* env_, HttpConnectionClass& cls_, JavaExceptionCheckScope & scope /* throws ProtocolException */, const char* url)
		: cls(cls_)
		, env(env_)
	{
		cls.Constructor.MakeNewInstance(env_, instance, scope, JavaRef(env_, url).ref);
	}

	~HttpConnectionInstance() {
		if (instance.ref) cls.Dispose(env, instance.ref);
	}

	void PutHeader(const char* name, const char* value) const {
		cls.PutHeader(env, instance, JavaRef(env, name).ref, JavaRef(env, value).ref);
	}

	void SetBody(JavaExceptionCheckScope& scope /* throws IOException if no connection */, const Array<UInt8>::View& data) const {
		cls.SetBody.CallWithScope(env, instance, scope, JavaRef(env, data).ref);
	}

	void SetMethod(const char* method) const {
		cls.SetMethod(env, instance, JavaRef(env, method).ref);
	}

	void SetTimeout(float secondsToConnect, float secondsPerRead) const {
		cls.SetTimeout(env, instance, secondsToConnect, secondsPerRead);
	}

	int PerformRequest(JavaExceptionCheckScope& scope /* throws IOException */) const {
		return cls.PerformRequest.CallWithScope(env, instance, scope);
	}

	void ReadResponseHeaders(Array<Pair<CString>>& out_result) const {
		JavaRef retVal (cls.ReadResponseHeaders(env, instance));

		JavaObjectToNative(env, retVal, out_result);
	}

	bool ReadResponseBody(JavaExceptionCheckScope& scope /* throws IOException */, int maxBytes, Array<UInt8>& out_result) const {
		JavaRef retVal (cls.ReadResponseBody.CallWithScope(env, instance, scope, maxBytes));

		if (retVal) {
			JavaObjectToNative(env, retVal, out_result);
			return true;
		}
		return false;
	}

	JNIEnv* env;
	HttpConnectionClass& cls;
	JavaRef<jobject> instance;
};

REFLEX_END_INTERNAL
