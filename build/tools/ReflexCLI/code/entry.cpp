#include "tasks.h"
#include "reflex_ext/bootstrap/console_app.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr CString::View kTargets[] = { "cmake", "xcode", "visual_studio", "android_studio" };

constexpr Key32 kDefaultTemplate = K32("default_template");
constexpr Key32 kDefaultVendor = K32("default_vendor");
constexpr Key32 kDefaultTargets = K32("default_targets");
constexpr Key32 kDefaultOutput = K32("default_output");

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
			CLI::ThrowError("invalid --target value");
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

Array <TemplateDefinition> GetTemplates()
{
	Array <TemplateDefinition> templates;

	auto root = Join(GetReflexPath(), L"templates/");

	if (!System::IsDirectory(root)) Bootstrap::CLI::ThrowError("no templates/ folder found");

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
			const auto print_overview_command_summary = [&std_out](CString::View name, CString::View description)
			{
				File::WriteLine(std_out, Join(kColourDefault, name, ' ', kColourDim, description, kColourDefault));
			};

			const auto print_arg = [&std_out](bool optional, CString::View name, CString::View description)
			{
				const auto kColourWhite = CLI::Detail::kColours[CLI::kColourWhite];
				const auto kColourName = optional ? kColourDim : kColourWhite;
				const auto kColourText = optional ? kColourDim : kColourDefault;

				File::WriteLine(std_out, Join(kColourName, name, ' ', kColourText, description, kColourDefault));
			};

			const auto show_overview = [&]()
			{
				CLI::Print(std_out, CLI::kColourBrightBlack, "Project Creation:");
				print_overview_command_summary("create", "create a new Reflex project from a template");
				print_overview_command_summary("list-templates", "list available Reflex project templates");
				print_overview_command_summary("list-targets", "list available project generation targets");
				print_overview_command_summary("set-default", "save default values used by the create command");
				print_overview_command_summary("get-defaults", "show saved default values for project creation");

				File::WriteLine(std_out);
				CLI::Print(std_out, CLI::kColourBrightBlack, "SDK Install:");
				print_overview_command_summary("install", "install or update the SDK");
				print_overview_command_summary("version", "show the installed SDK version");
				print_overview_command_summary("list-versions", "list available SDK versions");
				print_overview_command_summary("where", "show the install location");

				File::WriteLine(std_out);
				CLI::Print(std_out, CLI::kColourBrightBlack, "Build Phases:");
				print_overview_command_summary("build-resources", "build Reflex resource output for a source file");
				print_overview_command_summary("build-plist", "generate an Info.plist file for a supported target");

				File::WriteLine(std_out);
				File::WriteLine(std_out, Join(kColourDim, "use ", kColourDefault, "reflex help [command]", kColourDim, " for more info", kColourDefault));
			};

			const auto show_command_help = [&](CString::View command)
			{
				switch (MakeKey32(command))
				{
				case K32("create"):
					print_arg(false, "--template <id>", "the template to create");
					print_arg(false, "--vendor <vendor>", "the vendor name for the new project");
					print_arg(false, "--product <product>", "the product name for the new project");
					print_arg(false, "--output <folder>", "the destination folder for the generated project");
					print_arg(true, "--target <list>", "the project format(s) to generate");
					print_arg(true, "--overwrite false", "prevent overwriting template source files");
					return;

				case K32("install"):
					print_arg(false, "[version]", "the requested SDK version to install, leave unspecified for latest");
					print_arg(true, "--platforms <win|mac|android|ios[,..]>", "the platform packages to install");
					print_arg(true, "--path <folder>", "install the SDK to a specific location");
					print_arg(true, "--test true", "download and extract packages without moving files into place");
					return;

				case K32("set-default"):
					print_arg(false, "[at least one option required]", "set default values used by create");
					print_arg(true, "--template <id>", "set the default template");
					print_arg(true, "--vendor <vendor>", "set the default vendor");
					print_arg(true, "--target <list>", "set the default target(s) list");
					print_arg(true, "--output <folder>", "set the default output folder");
					return;

				case K32("build-resources"):
					print_arg(false, "--path <path>", "build Reflex resource output for a source file");
					return;

				case K32("build-plist"):
					print_arg(false, "--target <app|audioapp|ios_app|ios_audioapp|vst2|vst3|clap|au|auv3>", "the plist target type to generate");
					print_arg(false, "--output <path>", "the output plist file path");
					print_arg(false, "--product <name>", "the product name to embed");
					print_arg(false, "--bundle_id <id>", "the bundle identifier");
					print_arg(false, "--version <x.y.z>", "the product version string");
					print_arg(true, "--app_store_category <id>", "the App Store category identifier");
					print_arg(true, "--vendor <name>", "the vendor name to embed");
					print_arg(true, "--au_type <4cc>", "the Audio Unit type code");
					print_arg(true, "--au_subtype <4cc>", "the Audio Unit subtype code");
					print_arg(true, "--au_manufacturer <4cc>", "the Audio Unit manufacturer code");
					print_arg(true, "--au_description <text>", "the Audio Unit description text");
					return;

				case K32("version"):
				case K32("list-versions"):
				case K32("list-templates"):
				case K32("list-targets"):
				case K32("get-defaults"):
				case K32("where"):
					print_arg(true, "no arguments", "");
					return;

				default:
					show_overview();
					return;
				}
			};

			if (auto command = Data::GetCString(args, "value"))
			{
				show_command_help(command);
			}
			else
			{
				show_overview();
			}
		}
	},
	{
		.id = K32("set-default"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			bool set = false;

			if (auto pvalue = args.QueryProperty<Data::CStringProperty>("template"))
			{
				auto templates = GetTemplates();

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

				set = true;
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

				set = true;
			}

			if (auto pvalue = args.QueryProperty<Data::CStringProperty>("target"))
			{
				if (pvalue->value)
				{
					Data::SetCString(Bootstrap::global->prefs, kDefaultTargets, Merge(FindTargets(pvalue->value), ','));
				}
				else
				{
					Data::UnsetCString(Bootstrap::global->prefs, kDefaultTargets);
				}

				set = true;
			}

			if (auto value = CLI::GetFolder(args, "output", false))
			{
				Data::SetWString(Bootstrap::global->prefs, kDefaultOutput, value);

				set = true;
			}
			else if (auto pvalue = args.QueryProperty<Data::CStringProperty>("output"))
			{
				if (pvalue->value.Empty())
				{
					Data::UnsetWString(Bootstrap::global->prefs, kDefaultOutput);
					
					set = true;
				}
			}

			if (!set) CLI::ThrowError("nothing to set");
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
				File::WriteLine(std_out, Join("target=", value));
			}

			if (auto value = Data::GetWString(Bootstrap::global->prefs, kDefaultOutput))
			{
				File::WriteLine(std_out, Join(L"output=", value));
			}
		}
	},
	{
		.id = K32("where"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			File::WriteLine(std_out, GetReflexPath());
		}
	},
	{
		.id = K32("list-templates"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			auto templates = GetTemplates();

			if (CLI::GetBool(args, "detail"))
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
		.id = K32("install"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			Install(Data::GetCString(args, "value", "latest"), CLI::GetStringArray(args, "platforms"), CLI::GetFolder(args, "path", false), CLI::GetBool(args, "test"), std_out);
		}
	},
	{
		.id = K32("version"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			GetVersion(std_out);
		}
	},
	{
		.id = K32("list-versions"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			ListVersions(std_out);
		}
	},
	{
		.id = K32("create"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			constexpr auto get_default_target = []() -> CString::View
			{
				switch (System::kPlatform)
				{
				case System::kPlatformWindows:
					return "visual_studio";

				case System::kPlatformMacOS:
					return "xcode";

				default:
					return "cmake";
				}
			};

			auto reflex_path = GetReflexPath();

			CString template_arg = Data::GetCString(args, "template", Data::GetCString(Bootstrap::global->prefs, kDefaultTemplate));

			auto templates = GetTemplates();
			
			if (auto ptmpl = SearchTemplate(templates, template_arg))
			{
				auto targets = FindTargets(Data::GetCString(args, "target", Data::GetCString(Bootstrap::global->prefs, kDefaultTargets, get_default_target())));

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
							value = CLI::GetFolder(args, token.id, true);
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

				auto dest = CLI::GetFolder(args, "output", false);

				if (dest.Empty()) dest = Data::GetWString(Bootstrap::global->prefs, kDefaultOutput, System::GetCurrentDirectory());

				if (CaseInsensitive::eq(Left<true>(dest, reflex_path.GetSize()), reflex_path)) CLI::ThrowError("invalid output path, pass --output <folder> to a location outside the reflex repository");

				auto folder = CreateProject(*ptmpl, groups[0].c, groups[1].c, targets, dest, CLI::GetBool(args, "overwrite"), std_out);

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
			auto path = CLI::GetFilename(args, "path", true);

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

bool ReflexCLI::SaveGeneratedFile(const WString & path, Data::Archive::View data)
{
	auto handle = Make<System::FileHandle>(path, System::FileHandle::kModeOverwrite);

	bool written = handle->Write(data.data, data.size) == data.size;
	bool flushed = handle->Flush(true);
	
	return written && flushed;
}

Reflex::UInt8 Reflex::System::OnStart(const ArrayView <CString::View> & cmdline)
{
	auto global = AutoRelease(Bootstrap::Global::Acquire("Reflex++", "ReflexCLI", Bootstrap::Detail::ExtractProjectDir(__FILE__)));

	return Bootstrap::CLI::Dispatch(cmdline, ToView(ReflexCLI::kCommands), Bootstrap::CLI::kFlagPrintError);
}
