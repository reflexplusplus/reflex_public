#include "library.h"
#include "window.h"
#include "jni/[require].h"
#include <android/choreographer.h>
#include <sys/timerfd.h>

/**
 * Library implementation
 */

REFLEX_BEGIN_INTERNAL(Reflex::System::Android)

// Rendering (system clock) timing
constexpr bool kUseVsyncForSystemTimer = false;
constexpr long kRenderingInterval = long((1.0 / 60) * 1'000'000'000);
constexpr int kLooperTimerId = 0x100;
constexpr int kEventIdRunnable = 0x1001;

// Piece of code to be ran on a different thread
struct Runnable : Object {

#if REFLEX_DEBUG
		const char* Name;
#endif

		std::function<void()> Handle;

#if REFLEX_DEBUG
		Runnable(const char* name, std::function<void()> handle) : Name(name), Handle(std::move(handle)) {}
		Runnable(Runnable&& m) noexcept : Name(m.Name), Handle(std::move(m.Handle)) {}
#else
		Runnable(CString name, std::function<void()> handle) : Handle(handle) {}
		Runnable(Runnable&& m) : Handle(std::move(m.Handle)) {}
#endif

};

AtomicUInt8 g_disablerefresh = 0;

void HandleSystemClock(Library & library) {
	if (REFLEX_ATOMIC_READ(g_disablerefresh)) return;

	library.m_signals[kNotificationClock].Notify();
	if (auto window = Window::st_self) {
		window->ProcessPendingTasks();
	}
}

void HandleChoreographer(long, void* data) {
	HandleSystemClock(*Cast<Library>(data));

	AChoreographer_postFrameCallback(AChoreographer_getInstance(), HandleChoreographer, data);
}

int HandleTimer(int fd, int, void *data) {
	uint64_t expirations;
	read(fd, &expirations, sizeof(expirations));  // Consume the timer event

	HandleSystemClock(*Cast<Library>(data));
	return 1;
}

int HandleRunnable(int fd, int, void *data) {
	Runnable* msg;
	read(fd, &msg, sizeof(Runnable*));
	auto func = msg->Handle;
//	DEV_LOG("Processing message", msg->Name);
	func();
	Release(msg);
	return 1;
}

void RunSystemEventLoopIteration() {
	android_poll_source* pSource;
	int events;
	int result = ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&pSource));

	switch (result) {
		case ALOOPER_EVENT_ERROR:
			DEV_ERROR("ALooper_pollOnce returned an error")
			break;

		case ALOOPER_POLL_CALLBACK:
			break;

		default:
			if (pSource) pSource->process(Library::st_androidapp, pSource);
			break;
	}
}

UInt8 g_modifierflags = 0;

REFLEX_END_INTERNAL

void Reflex::System::Common::SetInstanceHandle(void * unused)
{
}

Reflex::System::Android::Library::Library()
{
	REFLEX_ASSERT(st_androidapp)

	// TODO: Florian -- InitKeyCodes();
}

Reflex::System::Android::Library::~Library() {
	if (m_timerfd >= 0) {
		ALooper_removeFd(st_androidapp->looper, m_timerfd);
	}
	if (pipe_fds[0]) {
		ALooper_removeFd(st_androidapp->looper, pipe_fds[0]);
		close(pipe_fds[0]);
	}
	if (pipe_fds[1]) close(pipe_fds[1]);

}

void Reflex::System::Android::Library::RunSystemEventLoop() {
	while (!st_androidapp->destroyRequested) {
		RunSystemEventLoopIteration();
	}
}

void Reflex::System::Android::Library::RunSystemEventLoop(bool handleInputAndDisplay, std::function<bool()> continueCondition) {
	if (!handleInputAndDisplay) REFLEX_ATOMIC_INC(g_disablerefresh, 1);

	while (!st_androidapp->destroyRequested && continueCondition()) {
		RunSystemEventLoopIteration();
	}

	if (!handleInputAndDisplay) REFLEX_ATOMIC_DEC(g_disablerefresh, 1);
}

Reflex::Pair<Reflex::System::iRect> Reflex::System::Android::Library::CurrentScreenInsets() const
{
	auto density = CurrentScreenDensity();
	iRect wholeScreen = {
		{ 0, 0 },
		{ ANativeWindow_getWidth(st_androidapp->window) / density,
		  ANativeWindow_getHeight(st_androidapp->window) / density }
	};
	iRect interactable = {
		{ m_interactableArea.origin.x / density,
		  m_interactableArea.origin.y / density },
		{ wholeScreen.size.w - m_interactableArea.size.w / density,
		  wholeScreen.size.h - m_interactableArea.size.h / density },
	};

	return { wholeScreen, interactable };
}

int Reflex::System::Android::Library::CurrentScreenDensity() const {
	return m_screendensity;
}

void Reflex::System::Android::Library::PostToReflexThread(const char* name, std::function<void()> handle) {
	Runnable* msg = REFLEX_CREATE(Runnable, name, std::move(handle));
	Retain(msg);
	write(pipe_fds[1], &msg, sizeof(Runnable*));
}

void Reflex::System::Android::Library::OnCodeReady(int screendensity) {
	m_screendensity = screendensity;

	if constexpr (kUseVsyncForSystemTimer) {
		// Request posting callback from the main thread (st_androidapp->looper)
		PostToReflexThread("InitChoreographer", [this] {
			AChoreographer_postFrameCallback(AChoreographer_getInstance(), HandleChoreographer, this);
		});
	}
	else {
		m_timerfd = timerfd_create(CLOCK_REALTIME, 0);
		REFLEX_ASSERT(m_timerfd >= 0)
		if (m_timerfd >= 0) {
			struct itimerspec spec = {
				{ 0, kRenderingInterval }, // Delay between firings in ns
				{ 0, kRenderingInterval }, // Delay before first call
			};

			timerfd_settime(m_timerfd, 0, &spec, nullptr);
			ALooper_addFd(st_androidapp->looper, m_timerfd, kLooperTimerId, ALOOPER_EVENT_INPUT, HandleTimer, this);
		}
	}

	// Create the pipe
	if (pipe(pipe_fds) != 0) {
		DEV_ERROR("Error creating ALooper pipe");
		return;
	}

	// Set non-blocking read
	fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);
	ALooper_addFd(st_androidapp->looper, pipe_fds[0], kEventIdRunnable, ALOOPER_EVENT_INPUT, HandleRunnable, this);
}

void Reflex::System::Android::Library::OnTextInputUpdate(const WString & text, UInt selection_start, UInt selection_length) {
	m_ontextinputdone(text, {selection_start, selection_length});
}

void Reflex::System::Android::Library::OnTextInputFinished() {
	m_ontextinputdone.Clear();
}



/**
 * System implementation
 */
const Reflex::System::Platform Reflex::System::kPlatform = kPlatformAndroid;

Reflex::Detail::Module::Member <Reflex::System::Android::Library> Reflex::System::Android::globals(Common::g_module);

Reflex::CString Reflex::System::GetOperatingSystemVersion() {
	Android::Jni::AttachedJavaEnv env;

	return Android::Jni::g_reflexActivityInstance->cls.GetOperatingSystemVersion(env);
}

Reflex::UInt64 Reflex::System::GetSystemID() {
	Android::Jni::AttachedJavaEnv env;

	auto androidId = Android::Jni::g_reflexActivityInstance->GetAndroidId(env);

	return Reflex::Detail::MakeHash<UInt64>(ToView(androidId));
}

Reflex::WString Reflex::System::GetPath(Path path) {
	REFLEX_USE(Android::Jni)
	// kPathApplicationData -> app bundle data (read-only)
	// kPathUserData -> app data -> files
	// kPathDesktop -> return {};
	// kPathTemp -> app data -> cache (on iOS: https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html)
	switch (path) {
		case kPathDesktop:
			DEV_WARN("GetPath(kPathDesktop) unavailable on mobile");
			return {};

		case kPathApplicationData: {    // %PROGRAMDATA%/
			auto dir = g_reflexActivityInstance->GetApplicationDir(AttachedJavaEnv());
			WString path = ToWString(dir);
			File::Detail::CorrectTrailingStroke(path);
			return path;
		}

		case kPathUserData: {        // %USERDIR%/AppData/Roaming
			auto dir = g_reflexActivityInstance->GetUserdataDir(AttachedJavaEnv());
			WString path = ToWString(dir);
			File::Detail::CorrectTrailingStroke(path);
			return path;
		}

		case kPathUserDocuments: {
			auto dir = g_reflexActivityInstance->GetUserdocsDir(AttachedJavaEnv());
			WString path = ToWString(dir);
			File::Detail::CorrectTrailingStroke(path);
			return path;
			}

		case kPathTemp: {
			auto dir = g_reflexActivityInstance->GetTempDir(AttachedJavaEnv());
			WString path = ToWString(dir);
			File::Detail::CorrectTrailingStroke(path);
			return path;
		}
	}
	return {};
}

Reflex::WString Reflex::System::GetExecutablePath()
{
	return {};
}

Reflex::Int32 Reflex::System::GetMaxPixelDensity() {
	return Android::globals->CurrentScreenDensity();
}

Reflex::Array<Reflex::System::iRect> Reflex::System::GetScreens() {
	return ToView(Android::globals->CurrentScreenInsets().a);
}

Reflex::CString Reflex::System::GetLanguage() {
	REFLEX_USE(Reflex::System::Android::Jni)
	return g_reflexActivityInstance->GetLanguage(AttachedJavaEnv());
}

bool Reflex::System::IsDarkTheme() {
	REFLEX_USE(Reflex::System::Android::Jni)
	return g_reflexActivityInstance->IsDarkTheme(AttachedJavaEnv());
}

Reflex::Float Reflex::System::GetFontScale() {
	REFLEX_USE(Reflex::System::Android::Jni)
	return g_reflexActivityInstance->GetFontSize(AttachedJavaEnv());
}

Reflex::UInt8 Reflex::System::GetModifierKeys() {
	// See: ModifierKey
	return Android::g_modifierflags;
}

Reflex::UInt32 Reflex::System::GetNumProcessor() {
	REFLEX_USE(Android::Jni)
	return g_reflexActivityInstance->GetNumProcessors(AttachedJavaEnv());
}

void Reflex::System::SetClipboard(const WString& string) {
	REFLEX_USE(Android::Jni)
	try {
		g_reflexActivityInstance->CopyTextToClipboard(AttachedJavaEnv(), string);
	}
	catch (const std::runtime_error& error) {
		DEV_LOG("Could not access clipboard", error.what());
	}
}

Reflex::WString Reflex::System::GetClipboard() {
	REFLEX_USE(Android::Jni)
	try {
		return g_reflexActivityInstance->GetTextFromClipboard(AttachedJavaEnv());
	}
	catch (const std::runtime_error& error) {
		DEV_LOG("Could not access clipboard", error.what());
	}
	return {};
}

bool Reflex::System::Open(const WString& path) {
	REFLEX_USE(Android::Jni)

	AttachedJavaEnv env;
	JavaExceptionCheckScope scope(env);
	auto& activity = *g_reflexActivityInstance;

	JavaRef jpath(env, path);
	bool result = activity.cls.launch.CallWithScope(env, activity.instance, scope, jpath.ref);

	return result && !scope.CheckAndClearException();
}

bool Reflex::System::Share(const ArrayView<WString>& items, const WString& text) {
	REFLEX_USE(Android::Jni)

	AttachedJavaEnv env;
	JavaExceptionCheckScope scope(env);
	auto& activity = *g_reflexActivityInstance;

	JavaRef<jobject> jitems;
	JavaFromNativeArray<jstring>(env, activity.stringClass.cls, items, jitems);

	JavaRef jtext(env, text);
	bool result = activity.cls.share.CallWithScope(env, activity.instance, scope, jitems.ref, jtext.ref);

	return result && !scope.CheckAndClearException();
}

Reflex::Float64 Reflex::System::GetElapsedTime() {
	using namespace std::chrono;
	auto duration = steady_clock::now().time_since_epoch();
	return duration_cast<nanoseconds>(duration).count() * 1e-9;
}

Reflex::TRef <Reflex::Object> Reflex::System::CreateListener(Notification id, void* client, void(*callback)(void*)) {
	return Android::globals->m_signals[id].Create(client, callback);
}
