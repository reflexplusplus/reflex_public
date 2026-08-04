#include "tasks.h"
#include "reflex_ext/bootstrap/console_app.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr CString::View kTargets[] = { "cmake", "xcode", "visual_studio", "android_studio", "all" };

WString GetReflexPathEx()
{
	if (auto path = Data::GetWString(Bootstrap::global->prefs, K32("reflex_path")))
	{
		return path;
	}
	else
	{
		return GetReflexPath();
	}
}

WString GetTemplatesFolder(const WString::View & repo_path)
{
	auto templates_root = Join(repo_path, L"templates/");

	if (!System::IsDirectory(templates_root)) Bootstrap::CLI::ThrowError("no templates/ folder found");

	return templates_root;
}

CString GetTemplateID(const TemplateDefinition & tmpl)
{
	return ToCString(File::SplitFilename(File::RemoveTrailingStroke(tmpl.folder)).b);
}

CString::View ValidateTargetsArg(const Data::PropertySet & args)
{
	auto targets = Data::GetCString(args, "targets", "cmake");

	for (auto & raw : Split(targets, ','))
	{
		auto value = Lowercase(Trim(raw));

		if (!Search(kTargets, value))
		{
			CLI::ThrowError("invalid --targets value");
		}
	}

	return targets;
}

Array <TemplateDefinition> GetTemplates(const WString & reflex_path)
{
	Array <TemplateDefinition> templates;

	auto root = GetTemplatesFolder(reflex_path);

	auto [folders, files] = File::List(root, true);

	for (auto & folder : folders)
	{
		auto tmpl = DecodeTemplate(OpenTemplateCfg(Join(root, folder.key)));

		if (tmpl.name.Empty()) continue;

		templates.Push(std::move(tmpl));
	}

	return templates;
}

const CLI::TaskDef kCommands[] =
{
	{
		.id = K32("help"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			constexpr Pair <CString::View> kCommands[] =
			{
				{ "create", "--template <id> --vendor <vendor> --product <product> [--targets <list>] --output <folder> [--overwrite false]" },
				{ "list-templates", "[--detail true]" },
				{ "list-targets", "" },
				{ "build-resources", "--path <path>" },
				{ "set-reflex-path", "--path <reflex-root>" },
				{ "get-reflex-path" },
			};

			for (auto & command : kCommands)
			{
				File::WriteLine(std_out, Join("reflex ", command.a, ' ', command.b));
			}
		}
	},
	{
		.id = K32("set-reflex-path"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto path = CLI::GetFolderArg(args, "path", true);

			Data::SetWString(Bootstrap::global->prefs, K32("reflex_path"), path);

			File::WriteLine(std_out, path);
		}
	},
	{
		.id = K32("get-reflex-path"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			File::WriteLine(std_out, GetReflexPathEx());
		}
	},
	{
		.id = K32("list-templates"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto templates = GetTemplates(GetReflexPathEx());

			if (CLI::GetBoolArg(args, "detail"))
			{
				Data::PropertySet root;

				auto keymap = Data::AcquireKeyMap(root);

				Data::RegisterKey(keymap, "folder");
				Data::RegisterKey(keymap, "name");
				Data::RegisterKey(keymap, "id");
				Data::RegisterKey(keymap, "token");
				Data::RegisterKey(keymap, "description");
				Data::RegisterKey(keymap, "input");
				Data::RegisterKey(keymap, "paths");
				Data::RegisterKey(keymap, "strings");

				UInt idx = 0;

				for (auto & tmpl : templates)
				{
					EncodeTemplate(tmpl, Data::AcquirePropertySet(root, idx++));
				}

				File::WriteBytes(std_out, Data::EncodePropertySet(Data::kPropertySheetFormat, root));
			}
			else
			{
				for (auto & tmpl : templates)
				{
					File::WriteLine(std_out, GetTemplateID(tmpl));
				}
			}
		}
	},
	{
		.id = K32("list-targets"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			for (auto target : ReverseSplice(kTargets, 1).a)
			{
				File::WriteLine(std_out, target);
			}
		}
	},
	{
		.id = K32("create"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto reflex_path = GetReflexPathEx();

			auto template_arg = Data::GetCString(args, "template");

			auto templates = GetTemplates(reflex_path);
			
			for (auto & tmpl : templates)
			{
				if (GetTemplateID(tmpl) == template_arg)
				{
					Tuple <bool, const Array <TokenDefinition> &, Array <Variable>> groups[] =
					{
						{ false, tmpl.strings },
						{ true, tmpl.paths }
					};

					for (auto & group : groups)
					{
						for (auto & token : group.b)
						{
							WString value;

							if (group.a)
							{
								value = CLI::GetFolderArg(args, token.id, true);
							}
							else 
							{
								if (auto arg = Data::GetCString(args, token.id))
								{
									value = ToWString(arg);
								}
								else
								{
									CLI::ThrowMissingArg(token.id, "<value>");
								}
							}

							group.c.Push({ token.token, value });
						}
					}

					auto dest = CLI::GetFolderArg(args, "output", false);

					if (dest.Empty())
					{
						dest = System::GetCurrentDirectory();

						if (CaseInsensitive::eq(Left<true>(dest, reflex_path.GetSize()), reflex_path)) CLI::ThrowMissingArg("output", "<folder>");
					}

					auto folder = CreateProject(tmpl, groups[0].c, groups[1].c, ValidateTargetsArg(args), dest, CLI::GetBoolArg(args, "overwrite"), std_out);

					File::WriteLine(std_out, Join(L"project created at ", folder));

					return;
				}
			}

			Array <CString> template_ids;

			for (auto & tmpl : templates)
			{
				template_ids.Push(GetTemplateID(tmpl));
			}

			CLI::ThrowMissingArg("template", Join('[', Merge(template_ids, '|'), ']'));
		}
	},
	{
		.id = K32("build-resources"),
		.fn = [](Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto path = CLI::GetFilenameArg(args, "path", true);

			Float progress = 0.0f;

			BuildResources(path, progress);
		}
	}
};

REFLEX_END_INTERNAL

Reflex::Output ReflexCLI::output("Reflex");

Reflex::UInt8 Reflex::System::OnStart(const ArrayView <CString::View> & cmdline)
{
	auto global = AutoRelease(Bootstrap::Global::Acquire("Reflex++", "ReflexCLI", Bootstrap::Detail::ExtractProjectDir(__FILE__)));

	return Bootstrap::CLI::Dispatch(cmdline, ToView(ReflexCLI::kCommands), Bootstrap::CLI::kFlagPrintError);
}
