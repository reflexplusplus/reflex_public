#include "jni/[require].h"
#include "jni/dialog.hpp"




REFLEX_BEGIN_INTERNAL(Reflex::System)

template<class FN>
struct CallbackTask : public Task
{
	CallbackTask(const Function <FN> & callback)
		: m_completed(false)
		, m_callback(callback)
	{
	}

	template<class ... VARGS> void Invoke(VARGS&& ... args) 
	{
		// Invoke only once, and if it's retained somewhere else (we always retain it in our API call)
		if (!m_completed && GetRetainCount() > 1) 
		{
			m_completed = true;
			
			m_callback(std::forward<VARGS>(args)...);
		}
	}

	bool Completed() const override 
	{
		return m_completed;
	}

	void Wait() override {}


private:
	
	bool m_completed;
	
	Function <FN> m_callback;
};

Reference <Task,false> g_showmessageboxtask;

bool st_keyboard_shown = false;

REFLEX_END_INTERNAL

Reflex::UInt32 Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags)
{
	if (g_showmessageboxtask)
	{
		DEV_LOG("Already showing a message box");

		return 0;
	}
	else
	{
		g_showmessageboxtask = ShowMessageBox(type, title, text, buttonflags, [](UInt32 ignoredResult)
		{
			g_showmessageboxtask.Clear();
		});

		return 1;
	}
}

Reflex::WString Reflex::System::GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir, const WString & filename)
{
	DEV_ERROR("Reflex::System::GetOpenPath unsupported on mobile");
	return {};
}

Reflex::WString Reflex::System::GetSavePath(const WString & title, const WString & filter, const WString & dir, const WString & filename)
{
	DEV_ERROR("Reflex::System::GetSavePath unsupported on mobile");
	return {};
}

Reflex::WString Reflex::System::GetFolder(const WString & title, const WString & root, bool cancreate)
{
	DEV_ERROR("Reflex::System::GetFolder unsupported on mobile, see documentation for alternate APIs");
	return {};
}

Reflex::Reference<Reflex::System::Task> Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags, const Function <void(UInt32 clickedButton)> & callback)
{
	REFLEX_USE(Android::Jni)

	//WString msg;
	//if (text) 
	//{
	//	const WChar delim = '\n';
	//
	//	for (auto & i : text) 
	//	{
	//		msg.Append(i);
	//	
	//		msg.Push(delim);
	//	}
	//	
	//	msg.Pop();
	//}

	AttachedJavaEnv env;
	
	auto task = Make<CallbackTask<void(UInt32)>>(callback);
	
	g_reflexActivityInstance->ShowMessageBox(env, title, Merge(text, L'\n'), [task](UInt32 result)
	{
		task->Invoke(result);
	});

	return task;
}

Reflex::Reference<Reflex::System::Task> Reflex::System::SelectExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, bool allowMultiple, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback) {
	REFLEX_USE(Android::Jni)
	REFLEX_ASSERT_EX(accessType == ExternalResourceRef::kAccessModeReadWrite || accessType == ExternalResourceRef::kAccessModeReadOnly, "Unsupported access type");

	AttachedJavaEnv env;
	auto task = Make<CallbackTask<void(const Array<Reference<System::ExternalResourceRef>>&)>>(callback);
	g_reflexActivityInstance->ShowFilePicker(env, mime_types, accessType, allowMultiple, {}, [task] (const Array<Array<UInt8>>& pickedFiles) {
		Array<Reference<System::ExternalResourceRef>> result;

		for (auto & file : pickedFiles) 
		{
			result.Push(REFLEX_CREATE(Android::ExternalResourceRef, file));
		}

		task->Invoke(result);
	});

	return task;
}

Reflex::Reference<Reflex::System::Task> Reflex::System::CreateExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, const WString::View& suggestedName, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback) {
	REFLEX_USE(Android::Jni)
	REFLEX_ASSERT_EX(accessType == ExternalResourceRef::kAccessModeCreateNew, "Unsupported access type");

	AttachedJavaEnv env;
	auto task = Make<CallbackTask<void(const Array<Reference<System::ExternalResourceRef>>&)>>(callback);
	g_reflexActivityInstance->ShowFilePicker(env, mime_types, accessType, false, suggestedName, [task] (const Array<Array<UInt8>>& pickedFiles) {
		Array<Reference<System::ExternalResourceRef>> result;

		for (auto & file : pickedFiles)
		{
			result.Push(REFLEX_CREATE(Android::ExternalResourceRef, file));
		}

		task->Invoke(result);
	});

	return task;
}

bool Reflex::System::ShowVirtualKeyboard(VirtualKeyboardInputType type, const WString& textbuffer, Pair<UInt> selection, const Function<void(const WString&, Pair<UInt>)>& ondone)
{
	Android::Jni::AttachedJavaEnv env;
	Android::globals->SetOnTextInputCallback(ondone);
	st_keyboard_shown = true;
	Android::Jni::g_reflexActivityInstance->EnableTextInput(env, true, type, textbuffer, selection);
	return true;
}

void Reflex::System::DismissVirtualKeyboard()
{
	Android::Jni::AttachedJavaEnv env;
	Android::globals->SetOnTextInputCallback({});
	if (SetFiltered(st_keyboard_shown, false)) {
		Android::Jni::g_reflexActivityInstance->EnableTextInput(env, false, kVirtualKeyboardInputNormal, {}, {});
	}
}
