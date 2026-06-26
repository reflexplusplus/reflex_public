#include "library.h"

#ifdef REFLEX_SYSTEM_AUDIO
#include "../common/instance/audioapp.cpp"
typedef Reflex::System::Common::AudioAppBase InstanceType;
#else
#include "../common/instance/app.cpp"
typedef Reflex::System::Common::NonAudioApp InstanceType;
#endif

int main(int argc, char * argv[])
{
	REFLEX_USE(Reflex);

	root_module.Init();

	Array <CString::View> cmdline;

	cmdline.SetSize(UInt(argc) - 1);

	REFLEX_LOOP(idx, cmdline.GetSize())
	{
		cmdline[idx] = CString::View(argv[idx + 1]);
	}

	TRef <InstanceType> instance = REFLEX_CREATE(InstanceType);

	if (instance->Initialise(cmdline) && System::Linux::globals->HasUI())
	{
		while (!System::Linux::globals->m_quit)
		{
			if (wl_display_dispatch(System::Linux::globals->m_display) == -1)
			{
				break;
			}
		}
	}

	instance->Deinitialise();

	root_module.Deinit();

	return 0;
}

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = Reflex::System::kEnvironmentTypeDesktopApp;

void Reflex::System::App::Quit()
{
	System::Linux::globals->m_quit = true;

	if (System::Linux::globals->m_display)
	{
		wl_display_flush(System::Linux::globals->m_display);
	}
}
