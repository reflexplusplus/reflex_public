#pragma once

#include "[include].h"
#include <android_native_app_glue.h>
#include "android_utils.hpp"
#include "../common/notification.h"
#include "../common/utf8.h"
#include "../common/instance/standalone.h"

REFLEX_NS(Reflex::System::Android)

struct Library {

	static inline android_app* st_androidapp = nullptr;
	Common::Signal m_signals[kNumNotification];
	iRect m_interactableArea;
	
	//lifetime
	Library();
	~Library();

	//internal

	// Runs the system events (blocking the script but allowing the app to be responsive to system events) until the condition is true
	static void RunSystemEventLoop();
	static void RunSystemEventLoop(bool handleInputAndDisplay, std::function<bool()> continueCondition);

	Pair<iRect> CurrentScreenInsets() const;
	int CurrentScreenDensity() const;

	static ALooper* GetLooperForMainThread() {
		REFLEX_ASSERT(st_androidapp && st_androidapp->looper)
		return st_androidapp->looper;
	}

	void OnCodeReady(int screendensity);
	// Runs the handler on the main reflex thread (as opposed to the main Java thread where JNI calls are made)
	void PostToReflexThread(const char* name, std::function<void()> handle);

	void SetOnTextInputCallback(const Function<void(const WString&, Pair<UInt>)>& ondone) { m_ontextinputdone = ondone; }
	void OnTextInputUpdate(const WString & text, UInt selection_start, UInt selection_length);
	void OnTextInputFinished();


private:

	int m_timerfd = -1;
	int pipe_fds[2] = {0, 0};  // pipe_fds[0] = read end, pipe_fds[1] = write end

	int m_screendensity = -1;

	Function <void(const WString&,Pair<UInt>)> m_ontextinputdone;
};

extern Reflex::Detail::Module::Member <Library> globals;

REFLEX_END

// ----------------------------------------

REFLEX_NS(Reflex::System::Android::Jni)

template<class R>
struct JavaCallbackWaiter {
	void NotifyResult(R result) {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_result = result;
	}

	R WaitForResult() {
		Library::RunSystemEventLoop(false, [this] {
			std::unique_lock<std::mutex> lock(m_mutex);
			return !m_result.operator bool();
		});
		Reseter reseter(m_result); // To prepare for a next call
		return m_result.value();
	}

private:
	std::mutex m_mutex;
	std::optional<R> m_result;

	struct Reseter {
		explicit Reseter(std::optional<R>& result) : m_result(result) {}
		~Reseter() { m_result.reset(); }

		std::optional<R>& m_result;
	};
};

REFLEX_END
