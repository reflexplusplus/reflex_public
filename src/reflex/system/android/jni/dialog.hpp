REFLEX_NS(Reflex::System::Android::Jni)

static Reflex::Function<void(const Reflex::Array<Reflex::Array<Reflex::UInt8>>& uris)> openFileDialogCallback;
static Reflex::Function<void(Reflex::UInt32 result)> showMessageBoxCallback;

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_notifyResultFromOpenFileDialog(JNIEnv *env, jobject thiz, jobjectArray result) {
	REFLEX_USE(Reflex)
	Array<Array<UInt8>> uris;
	System::Android::Jni::JavaObjectToNativeArray(env, result, uris);
	System::InvokeAndClear(openFileDialogCallback, uris);
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_notifyResultFromMessageBox(JNIEnv *env, jobject thiz, jint result) {
	Reflex::System::InvokeAndClear(showMessageBoxCallback, (Reflex::UInt32) result);
}

void ReflexActivityInstance::ShowMessageBox(JNIEnv *env, const WString::View& title, const WString::View& text, Function<void(UInt32 clickedButton)>&& callback) const {
	if (showMessageBoxCallback) {
		callback(0);
		return;
	}

	JavaExceptionCheckScope scope(env);

	showMessageBoxCallback = std::move(callback);
	cls.showMessageBox.CallWithScope(env, instance, scope, JavaRef(env, title).ref, JavaRef(env, text).ref);

	if (scope.CheckAndClearException()) {
		InvokeAndClear(showMessageBoxCallback, 0U);
	}
}

void ReflexActivityInstance::ShowFilePicker(
	JNIEnv *env, const ArrayView<WString>& mime_types,
	ExternalResourceRef::AccessMode accessType,
	bool allowMultiple, const WString::View& suggestedName,
	Function<void(const Array<Array<UInt8>>& uris)>&& callback) const
{
	// Already launched
	if (openFileDialogCallback) {
		callback(Array<Array<UInt8>>());
		return;
	}

	JavaExceptionCheckScope scope(env);
	JavaRef javaMimeTypes;
	JavaFromNativeArray<jstring>(env, g_reflexActivityInstance->stringClass.cls, mime_types, javaMimeTypes);

	openFileDialogCallback = std::move(callback);
	cls.showFilePicker.CallWithScope(env, instance, scope, javaMimeTypes.ref, accessType, allowMultiple, JavaRef(env, suggestedName).ref);

	if (scope.CheckAndClearException()) {
		InvokeAndClear(openFileDialogCallback, Array<Array<UInt8>>());
	}
}

REFLEX_END
