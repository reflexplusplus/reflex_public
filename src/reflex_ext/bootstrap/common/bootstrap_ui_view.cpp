#include "[require].h"




//
//impl

Reflex::Bootstrap::View::View(File::PersistentPropertySet & session, Key32 chunk_id, UInt16 chunk_version, WString::View stylesheet_path)
	: Streamable(session, chunk_id, chunk_version)
	, m_prefs_listener(global->prefs->CreateListener([this](File::PersistentPropertySet::Notification n, Key32 context)
	{
		Update();
	}))
	, m_stylesheet_path(stylesheet_path)
{
#if REFLEX_DEBUG
	//make sure stylesheet is already in resourcepool here, before IDE builder view updates, so it can restore focus
	GLX::RetrieveStyleSheet(stylesheet_path, AutoRelease(Detail::g_create_stylesheet_options()));
#endif

	GLX::SetFlow(*this, GLX::kFlowY);

	EnableOnAttachDetachWindow();

	EnableOnClock();
}

void Reflex::Bootstrap::View::OnReset(Key32 context)
{
	GLX::Core::Context ctx;

	OnResetState(context);

	Update();
}

void Reflex::Bootstrap::View::OnRestore(Data::Archive::View & stream, Key32 context)
{
	GLX::Core::Context ctx;

	OnRestoreState(stream, context);

	Update();
}

void Reflex::Bootstrap::View::OnStore(Data::Archive & stream) const
{
	GLX::Core::Context ctx;

	OnStoreState(stream);
}

void Reflex::Bootstrap::View::OnAttachWindow()
{
	Detail::SetStyle(*this, m_stylesheet_path, {}, Detail::g_create_stylesheet_options);

	Streamable::RestoreState();
}

void Reflex::Bootstrap::View::OnDetachWindow()
{
	Streamable::StoreState();
}

void Reflex::Bootstrap::View::OnClock(Float)
{
	if (m_monitor.Poll())
	{
		Update();
	}
}

bool Reflex::Bootstrap::View::OnEvent(GLX::Object & src, GLX::Event & e)
{
#if REFLEX_DEBUG
	if (e.id == GLX::kKeyDown)
	{
		switch (GLX_KEY_CODE(GLX::GetKeyCode(e), GLX::GetModifierKeys(e)))
		{
		case GLX_KEY_CODE(GLX::kKeyCodeD, GLX::kModifierKeyShift | GLX::kModifierKeyPrimary):
			global->EnableIde(!global->IdeEnabled());
			return true;

		case GLX_KEY_CODE(GLX::kKeyCodeF5, GLX::kModifierKeyShift):
			GLX::Restart();
			return true;

		default:
			break;
		}
	}
#endif

	return GLX::Object::OnEvent(src, e);
}
