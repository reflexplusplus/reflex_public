#include "../module.h"




//
//Entry

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = kEnvironmentTypeConsoleApp;

int main(int argc, char * argv[])
{
	REFLEX_USE(Reflex);

	root_module.Init();

	Array <CString::View> args;

	if (auto narg = argc - 1)
	{
		auto pargview = Extend(args, narg).data;

		REFLEX_LOOP_PTR(argv + 1, parg, narg)
		{
			*pargview++ = (const char*)(*parg);
		}
	}

	auto exit_code = System::OnStart(args);

	root_module.Deinit();

	return int(exit_code);
}
