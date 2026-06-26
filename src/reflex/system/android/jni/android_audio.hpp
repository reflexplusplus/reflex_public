REFLEX_NS(Reflex::System::Android::Jni)

static Reflex::System::Android::Jni::JavaCallbackWaiter<bool> recordingPermissionWaiter;

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onRecordingPermissionResult(JNIEnv *env, jobject thiz, jboolean granted) {
	recordingPermissionWaiter.NotifyResult(granted);
}

struct AudioDeviceClass : Object {
	explicit AudioDeviceClass(JNIEnv* env)
		: AudioDeviceClass(env, JavaRef(findClass(env, "com/reflex_multimedia/reflex/sdk/AudioDevice")))
	{}

	AudioDeviceClass(JNIEnv* env, jclass clazz)
		: id(env->GetFieldID(clazz, "id", JavaSignature<int>))
		, type(env->GetFieldID(clazz, "type", JavaSignature<int>))
		, numChannels(env->GetFieldID(clazz, "numChannels", JavaSignature<int>))
		, isInput(env->GetFieldID(clazz, "isInput", JavaSignature<bool>))
		, isDefault(env->GetFieldID(clazz, "isDefault", JavaSignature<bool>))
		, productName(env->GetFieldID(clazz, "productName", JavaSignature<CString>))
		, friendlyName(env->GetFieldID(clazz, "friendlyName", JavaSignature<CString>))
		, sampleRates(env->GetFieldID(clazz, "sampleRates", JavaSignature<Array<float>>))
	{}

	static constexpr Key32 key = "AudioDeviceClass";

	jfieldID id, type, numChannels, isInput, isDefault, productName, friendlyName, sampleRates;
};

struct AudioDeviceInstance : Object {
	AudioDeviceInstance(JNIEnv* env, jobject instance)
		: cls(g_reflexActivityInstance->GetClass<AudioDeviceClass>(env))
		, id(env->GetIntField(instance, cls.id))
		, type(env->GetIntField(instance, cls.type))
		, numChannels(env->GetIntField(instance, cls.numChannels))
		, isInput(env->GetBooleanField(instance, cls.isInput))
		, isDefault(env->GetBooleanField(instance, cls.isDefault))
	{
		JavaRef javaObj;
		javaObj.SetAsOwned(env->GetObjectField(instance, cls.productName));
		JavaObjectToNative(env, jstring(javaObj.ref), productName);

		javaObj.SetAsOwned(env->GetObjectField(instance, cls.friendlyName));
		JavaObjectToNative(env, jstring(javaObj.ref), friendlyName);

		javaObj.SetAsOwned(env->GetObjectField(instance, cls.sampleRates));
		JavaObjectToNative(env, javaObj.ref, sampleRates);
	}

	AudioDeviceClass& cls;
	int id, type, numChannels;
	bool isInput, isDefault;
	CString productName, friendlyName;
	Array<float> sampleRates;
};

Array<Reference<AudioDeviceInstance>> ReflexActivityInstance::GetAudioDevices(JNIEnv* env, const WString::View& defaultDeviceName) {
	Array<Reference<AudioDeviceInstance>> result;
	JavaRef<jobjectArray> retVal{
		cls.getAudioDevices(env, instance, JavaRef(env, defaultDeviceName).ref)
	};

	auto len = env->GetArrayLength(retVal);
	result.Allocate(len);
	REFLEX_LOOP_TYPE(jsize, i, len) {
		result.Push(REFLEX_CREATE(AudioDeviceInstance, env, env->GetObjectArrayElement(retVal, i)));
	}
	return result;
}

bool ReflexActivityInstance::RequestRecordingPermission(JNIEnv *env) {
	{	JavaExceptionCheckScope scope(env);
		m_got_recording_permission = cls.requestRecordingPermission(env, instance);
	}

	if (m_got_recording_permission) return true;

	return (m_got_recording_permission = recordingPermissionWaiter.WaitForResult());
}

REFLEX_END
