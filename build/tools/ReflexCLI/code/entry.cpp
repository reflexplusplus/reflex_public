#include "tasks.h"
#include "reflex_ext/bootstrap/console_app.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr CString::View kTargets[] = { "cmake", "xcode", "visual_studio", "android_studio" };

constexpr Key32 kDefaultTemplate = K32("default_template");
constexpr Key32 kDefaultVendor = K32("default_vendor");
constexpr Key32 kDefaultTargets = K32("default_targets");
constexpr Key32 kDefaultOutput = K32("default_output");

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

Array <CString> FindTargets(const CString::View & arg)
{
	Array <CString> targets;

	for (auto & raw : Split(arg, ','))
	{
		auto value = Trim(raw);

		if (StringCompare::eq(value, "all"))
		{
			targets.Clear();

			for (auto target : kTargets)
			{
				targets.Push(target);
			}

			return targets;
		}
		else if (Search<StringCompare>(kTargets, value))
		{
			targets.Push(value);
		}
		else
		{
			CLI::ThrowError("invalid --targets value");
		}
	}

	return targets;
}

const TemplateDefinition * SearchTemplate(const ArrayView <TemplateDefinition> & templates, const CString::View & value)
{
	for (auto & tmpl : templates)
	{
		if (StringCompare::eq(GetTemplateID(tmpl), value)) return &tmpl;
	}

	return nullptr;
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
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			constexpr Pair <CString::View> kCommands[] =
			{
				{ "create", "--template <id> --vendor <vendor> --product <product> [--targets <list>] --output <folder> [--overwrite false]" },
				{ "list-templates", "" },
				{ "list-targets", "" },
				{ "build-resources", "--path <path>" },
				{ "build-plist", "--target <app|audioapp|ios_app|ios_audioapp|vst2|vst3|clap|au|auv3> --output <path> --product <name> --bundle_id <id> --version <x.y.z> [--app_store_category <id>] [--vendor <name>] [--au_type <4cc>] [--au_subtype <4cc>] [--au_manufacturer <4cc>] [--au_description <text>]" },
				{ "set-default", "[--template <id>] [--vendor <vendor>] [--targets <list>] [--output <folder>]" },
				{ "get-defaults", "" },
				{ "set-reflex-path", "--path <reflex-root>" },
				{ "get-reflex-path", "" },
			};

			const auto kColourDefault = CLI::Detail::kColours[CLI::kColourWhite];
			const auto kColourBrightBlack = CLI::Detail::kColours[CLI::kColourBrightBlack];

			for (auto & command : kCommands)
			{
				File::WriteLine(std_out, Join(kColourDefault, command.a, ' ', kColourBrightBlack, command.b, kColourDefault));
			}
		}
	},
	{
		.id = K32("set-default"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			bool any = false;

			if (auto pvalue = args.QueryProperty<Data::CStringProperty>("template"))
			{
				auto templates = GetTemplates(GetReflexPathEx());

				if (SearchTemplate(templates, pvalue->value))
				{
					Data::SetCString(Bootstrap::global->prefs, kDefaultTemplate, pvalue->value);
				}
				else if (pvalue->value.Empty())
				{
					Data::UnsetCString(Bootstrap::global->prefs, kDefaultTemplate);
				}
				else
				{
					CLI::ThrowError("invalid template id");
				}

				any = true;
			}

			if (auto pvalue = args.QueryProperty<Data::CStringProperty>("vendor"))
			{
				if (pvalue->value)
				{
					Data::SetCString(Bootstrap::global->prefs, kDefaultVendor, pvalue->value);
				}
				else
				{
					Data::UnsetCString(Bootstrap::global->prefs, kDefaultVendor);
				}

				any = true;
			}

			if (auto pvalue = args.QueryProperty<Data::CStringProperty>("targets"))
			{
				if (pvalue->value)
				{
					Data::SetCString(Bootstrap::global->prefs, kDefaultTargets, Merge(FindTargets(pvalue->value), ','));
				}
				else
				{
					Data::UnsetCString(Bootstrap::global->prefs, kDefaultTargets);
				}

				any = true;
			}

			if (auto value = CLI::GetFolderArg(args, "output", false))
			{
				Data::SetWString(Bootstrap::global->prefs, kDefaultOutput, value);

				any = true;
			}
			else if (auto pvalue = args.QueryProperty<Data::CStringProperty>("output"))
			{
				if (pvalue->value.Empty()) Data::UnsetWString(Bootstrap::global->prefs, kDefaultOutput);
			}

			if (!any) CLI::ThrowError("nothing to set");
		}
	},
	{
		.id = K32("get-defaults"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			if (auto value = Data::GetCString(Bootstrap::global->prefs, kDefaultTemplate))
			{
				File::WriteLine(std_out, Join("template=", value));
			}

			if (auto value = Data::GetCString(Bootstrap::global->prefs, kDefaultVendor))
			{
				File::WriteLine(std_out, Join("vendor=", value));
			}

			if (auto value = Data::GetCString(Bootstrap::global->prefs, kDefaultTargets))
			{
				File::WriteLine(std_out, Join("targets=", value));
			}

			if (auto value = Data::GetWString(Bootstrap::global->prefs, kDefaultOutput))
			{
				File::WriteLine(std_out, Join(L"output=", value));
			}
		}
	},
	{
		.id = K32("set-reflex-path"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto path = CLI::GetFolderArg(args, "path", true);

			Data::SetWString(Bootstrap::global->prefs, K32("reflex_path"), path);

			File::WriteLine(std_out, path);
		}
	},
	{
		.id = K32("get-reflex-path"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			File::WriteLine(std_out, GetReflexPathEx());
		}
	},
	{
		.id = K32("list-templates"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
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
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			for (auto target : kTargets)
			{
				File::WriteLine(std_out, target);
			}
		}
	},
	{
		.id = K32("create"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto reflex_path = GetReflexPathEx();

			CString template_arg = Data::GetCString(args, "template", Data::GetCString(Bootstrap::global->prefs, kDefaultTemplate));

			auto templates = GetTemplates(reflex_path);
			
			if (auto ptmpl = SearchTemplate(templates, template_arg))
			{
				auto targets = FindTargets(Data::GetCString(args, "targets", Data::GetCString(Bootstrap::global->prefs, kDefaultTargets, "cmake")));

				Tuple <bool, const Array <TokenDefinition> &, Array <Variable>> groups[] =
				{
					{ false, ptmpl->strings },
					{ true, ptmpl->paths }
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
							else if (StringCompare::eq(token.id, "vendor"))
							{
								if (auto vendor = Data::GetCString(Bootstrap::global->prefs, kDefaultVendor))
								{
									value = ToWString(vendor);
								}
								else
								{
									CLI::ThrowMissingArg(token.id, "<value>");
								}
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

				if (dest.Empty()) dest = Data::GetWString(Bootstrap::global->prefs, kDefaultOutput, System::GetCurrentDirectory());

				if (CaseInsensitive::eq(Left<true>(dest, reflex_path.GetSize()), reflex_path)) CLI::ThrowError("invalid output path, pass --output <folder> to a location outside the reflex repository");

				auto folder = CreateProject(*ptmpl, groups[0].c, groups[1].c, targets, dest, CLI::GetBoolArg(args, "overwrite"), std_out);

				File::WriteLine(std_out, Join(L"project created at ", folder));

				return;
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
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto path = CLI::GetFilenameArg(args, "path", true);

			Float progress = 0.0f;

			BuildResources(path, progress);
		}
	},
	{
		.id = K32("build-plist"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			BuildPlist(args, std_out);
		}
	},
#if REFLEX_DEBUG
	{
		.id = K32("test-progress-bar"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto task = AutoRelease(System::Thread::Create([&args, &std_out]()
			{
				auto steps = ToInt32(Data::GetCString(args, "steps", "40"));
				auto delay_ms = ToInt32(Data::GetCString(args, "delay_ms", "50"));

				if (steps <= 0) CLI::ThrowError("steps must be > 0");
				if (delay_ms < 0) CLI::ThrowError("delay_ms must be >= 0");

				CLI::Await(std_out, "testing progress bar...", true, {}, [steps, delay_ms](CLI::TaskContext & ctx)
				{
					for (Int32 i = 0; i < steps; ++i)
					{
						ctx.SetProgress(Float32(i + 1) / Float32(steps));

						System::SuspendThread(UInt32(delay_ms));
					}
				});

				CLI::Print(std_out, CLI::kColourGreen, "progress bar test completed");
			}));
		}
	}
#endif
};

REFLEX_END_INTERNAL

Reflex::Output ReflexCLI::output("Reflex");

void ReflexCLI::ThrowError(CString::View msg, CString::View error)
{
	Bootstrap::CLI::ThrowError(Join(msg, ':', ' ', error));
}

void ReflexCLI::ThrowError(CString::View msg, WString::View error)
{
	ThrowError(msg, ToCString(error));
}

Reflex::UInt8 Reflex::System::OnStart(const ArrayView <CString::View> & cmdline)
{
	auto global = AutoRelease(Bootstrap::Global::Acquire("Reflex++", "ReflexCLI", Bootstrap::Detail::ExtractProjectDir(__FILE__)));

	return Bootstrap::CLI::Dispatch(cmdline, ToView(ReflexCLI::kCommands), Bootstrap::CLI::kFlagPrintError);
}
