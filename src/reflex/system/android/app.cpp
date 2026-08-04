#include "library.h"
#include "jni/[require].h"

#ifdef REFLEX_SYSTEM_AUDIO
#include "../common/instance/audioapp.cpp"
#include "audio_app.cpp"
typedef Reflex::System::Android::AudioApp AppType;
#else
#include "../common/instance/app.cpp"
typedef Reflex::System::Common::NonAudioApp AppType;
#endif

REFLEX_BEGIN_INTERNAL(Reflex::System::Android)

CString pending_deeplink;

void EmitDeepLink(App & app)
{
	Cast<AppType>(app)->GetGlobal()->SetProperty(K32("DeepLinkUrl"), New<ObjectOf<CString>>(pending_deeplink));

	pending_deeplink.Clear();
}
// The app can have three states:
// 1) No graphics context available; library might be running, but graphics will have to be created from zero
// 2) Graphics available but context unassigned yet (or de-assigned as the window was destroyed)
// 3) Graphics available and context assigned (normal state as the app is running and shown on the screen)
// Normally, putting the app in background transitions from 3 to 2, but in some circumstances (low memory), it might further transition from 2 to 1, meaning that the editor gets closed and will have to be recreated.
void HandleApplicationCommand(android_app* app, int32_t cmd) {
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			// Window can be recreated: no need to initialize twice
			if (!AppType::Get()) {
				REFLEX_ASSERT_EX(globals->CurrentScreenDensity() > 0, "APP_CMD_INIT_WINDOW might have been called before onCodeReady");

				// Now we are able to use opengl with this window, so we can start the application
				auto instance = REFLEX_CREATE(AppType);
				instance->Initialise<false>({}); // Sets it as AppType::Get()
			}

			AppType::Get()->OpenEditor();
			Window::st_self->UpdateWindowOnCreation();

			if (pending_deeplink) EmitDeepLink(*AppType::Get());
			break;

		case APP_CMD_LOW_MEMORY:
			// Unusable as this gets called every single time when putting the app in background...
			break;

		case APP_CMD_GAINED_FOCUS:
			Window::st_self->OnSetFocus();
			break;

		case APP_CMD_LOST_FOCUS:
			Window::st_self->OnLoseFocus();
			break;

		case APP_CMD_TERM_WINDOW:
			if (auto app = AppType::Get()) app->CloseEditor();
			break;

		case APP_CMD_DESTROY:
			if (auto app = AppType::Get()) app->Deinitialise();
			break;
	}
}

REFLEX_END_INTERNAL

/**
 * Expected execution order:
 * 1) android_main (from MainActivity.super.onCreate). This will create the library. Main thread.
 * 2) onCodeReady (from MainActivity.onCreate). This will provide additional high level informations computed from Kotlin, instead of having to do it tediously with native code. We are in a different thread (Java).
 * 3) APP_CMD_INIT_WINDOW: the window is ready. Coming later. That's only at that point that we can create an opengl context. That's why we will be waiting for that until we create an instance of the client code. Main thread.
 */
REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onCodeReady(JNIEnv* env, jobject thiz, jint density) {
	Reflex::System::Android::Jni::initJava(env, thiz);

	Reflex::System::Android::globals->OnCodeReady(density);
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onDeepLink(JNIEnv* env, jobject thiz, jstring url) {
	REFLEX_USE(Reflex::System::Android)

	Jni::JavaObjectToNative(env, url, pending_deeplink);

	if (auto app = AppType::Get()) EmitDeepLink(*app);
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onAudioDevicesChanged(JNIEnv* env, jobject thiz) {
#ifdef REFLEX_SYSTEM_AUDIO
	REFLEX_USE(Reflex::System::Android)
	globals->PostToReflexThread("onAudioDevicesChanged", [] {
		if (auto app = Reflex::Cast<AppType>(AppType::Get())) app->OnAudioDevicesChanged();
	});
#endif
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onWindowInsetsChanged(JNIEnv* env, jobject thiz, jintArray interactable) {
	REFLEX_USE(Reflex::System::Android)

	Reflex::Array<int> interactableInsetsArray;
	Reflex::System::Android::Jni::JavaObjectToNative(env, interactable, interactableInsetsArray);
	REFLEX_ASSERT(interactableInsetsArray.GetSize() == 4)

	auto interactableInsets = *Reflex::Reinterpret<Reflex::System::iRect>(interactableInsetsArray.GetData());

	globals->PostToReflexThread("onWindowInsetsChanged", [interactableInsets] {
		globals->m_interactableArea = interactableInsets;

		if (auto window = Window::st_self) {
			window->ScheduleWindowRectChanged();
		}
	});
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onTextInputUpdate(JNIEnv* env, jobject thiz, jstring text, jint selection_start, jint selection_end) {
	REFLEX_USE(Reflex)
	REFLEX_USE(Reflex::System::Android)

	WString ctext;
	Jni::JavaObjectToNative(env, text, ctext);

	globals->PostToReflexThread("OnTextInputUpdate", [ctext, selection_start, selection_end] {
		globals->OnTextInputUpdate(ctext, UInt(selection_start), UInt(selection_end - selection_start));
	});
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onTextInputFinished(JNIEnv* env, jobject thiz) {
	REFLEX_USE(Reflex::System::Android)

	globals->PostToReflexThread("OnTextInputFinished", [] {
		globals->OnTextInputFinished();
	});
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_onUpdateTheme(JNIEnv *env, jobject thiz) {
	REFLEX_USE(Reflex::System)
	REFLEX_USE(Reflex::System::Android)

	globals->PostToReflexThread("OnUpdateTheme", [] {
		globals->m_signals[kNotificationChangeDisplays].Notify();
	});
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_devLog(JNIEnv* env, jobject thiz, jstring s) {
	Reflex::CString cstr;
	Reflex::System::Android::Jni::JavaObjectToNative(env, s, cstr);
	DEV_LOG(cstr);
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_devWarn(JNIEnv* env, jobject thiz, jstring s) {
	Reflex::CString cstr;
	Reflex::System::Android::Jni::JavaObjectToNative(env, s, cstr);
	DEV_WARN(cstr);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_getSupportedOrientations(JNIEnv* env, jobject thiz) {
	return 0;
}

REFLEX_JNIEXPORT(void) Java_com_reflex_1multimedia_reflex_sdk_ReflexActivity_imeInsertText(JNIEnv* env, jobject thiz, jint chars_to_erase, jstring chars_to_insert) {
	REFLEX_USE(Reflex::System::Android)
	Reflex::WString native_chars;
	Jni::JavaObjectToNative(env, chars_to_insert, native_chars);

	globals->PostToReflexThread("HandleImeInput", [chars_to_erase, native_chars] {
		if (auto window = Window::st_self) {
			window->HandleImeInput(chars_to_erase, native_chars);
		}
	});
}

void android_main(android_app* app) {
	REFLEX_USE(Reflex)
	REFLEX_USE(Reflex::System::Android)

	Library::st_androidapp = app;

	Reflex::root_module.Init();

	app->onAppCmd = HandleApplicationCommand;
	app->onInputEvent = [] (android_app* app, AInputEvent* event) {
		if (auto window = Window::st_self) {
			return window->HandleNextInputEvent(event);
		}
		return 0;
	};

	Library::RunSystemEventLoop();

	Jni::deinitJava();
 	Reflex::root_module.Deinit();
	Library::st_androidapp = nullptr;
}

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = kEnvironmentTypeMobileApp;

void Reflex::System::App::Quit() {
	// Works, but we've chosen to support it either everywhere either nowhere
//	Android::Jni::AttachedJavaEnv env;
//	Android::Jni::g_reflexActivityInstance->FinishActivity(env);
	DEV_WARN("System::App::Quit not supported on mobile");
}