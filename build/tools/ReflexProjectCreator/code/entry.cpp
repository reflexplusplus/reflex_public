#include "app.h"
#include "view.h"





Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
#if REFLEX_DEBUG
	constexpr auto get_agent_args = [](ArrayView <CString::View> cmdline)
	{
		auto request_path = Join(Bootstrap::Detail::ExtractProjectDir(__FILE__), L"reflex_cmdline.json");

		if (auto request = File::Open(request_path))
		{
			File::Delete(request_path);

			return Data::DecodePropertySet(Data::kJsonFormat, request);
		}

		return Bootstrap::ParseCmdlineArgs(cmdline, true);
	};

	auto args = get_agent_args(cmdline);

	if (Data::GetBool(args, "terminate-on-assert"))
	{
		System::Detail::DebugBreak = [](const char * msg)
		{
			auto file = Output::GetLogFile();

			file->Flush(true);

			File::WriteLine(file, "*** REFLEX_ASSERT ***");
			File::WriteLine(file, msg);

			System::Detail::EnumerateStackTrace(file.Adr(), [](void * pfile, UInt frame, const void * address, const char * symbol)
			{
				auto file = Cast<System::FileHandle>(pfile);

				auto buffer = Reflex::Detail::DebugJoin(" ", frame, address, symbol);

				File::WriteLine(*file, buffer);
			});

			file->Flush(true);

			System::Detail::Terminate(1);
		};
	}
#endif

	Bootstrap::PublishAppView<ReflexProjectCreator::App, ReflexProjectCreator::View>(config);

	auto global = Bootstrap::StartApp<ReflexProjectCreator::App>
	(
		config,
		"Reflex++",
		"Project Creator",
		K32("ReflexProjectCreator"),
		__FILE__
	);

#if REFLEX_DEBUG
	if (auto delay = Data::GetFloat32(args, "auto-quit", ToFloat32(Data::GetInt32(args, "auto-quit"))))
	{
		SetAbstractProperty(global, "auto-quit", Async::CreatePeriodicClock(delay, []()
		{
			System::App::Quit();
		}));
	}
#endif

	return global;
}
