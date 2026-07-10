#include "tasks.h"
#include "reflex_ext/bootstrap/console_app.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr CString::View kVisualStudio = "visual_studio";
constexpr CString::View kXcode = "xcode";
constexpr CString::View kTargets[] = { "cmake", kXcode, kVisualStudio, "android_studio" };

CString GetTemplateID(const TemplateDefinition & tmpl)
{
	return ToCString(File::SplitFilename(File::RemoveTrailingStroke(tmpl.folder)).b);
}

WString PromptValue(System::FileHandle & std_in, System::FileHandle & std_out, WString::View default_value = {})
{
	CString prompt;
	
	if (default_value) prompt  = Join(kColourDim, "[", ToCString(default_value), "]: ", kColourDefault);

	std_out.Write(prompt.GetData(), prompt.GetSize());

	std_out.Flush(true);

	char buffer[1024];

	auto n = std_in.Read(buffer, GetArraySize(buffer));

	const char * pbuffer = buffer;

	CString::View cbuffer = { pbuffer, n };

	cbuffer = Data::Detail::ReadLine(cbuffer);

	cbuffer = Trim(cbuffer);

	WString trimmed = ToWString(cbuffer);

	return trimmed ? trimmed : Join(default_value);
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

void Create(const Data::PropertySet & args, System::FileHandle & std_out)
{
	constexpr auto get_default_target = []() -> CString::View
	{
		switch (System::kPlatform)
		{
		case System::kPlatformWindows:
			return kVisualStudio;

		case System::kPlatformMacOS:
			return kXcode;
		
		default:
			return "";
		}
	};

	auto reflex_path = GetReflexPath();
	
	auto std_in = Make<System::FileHandle>(System::FileHandle::kStandardStreamIn);

	const auto prompt_required = [&std_out, std_in](WString::View default_value = {}) -> WString
	{
		while (true)
		{
			if (auto value = PromptValue(*std_in, std_out, default_value))
			{
				return value;
			}
		}
	};

	auto prefs = Data::AcquirePropertySet(Bootstrap::global->prefs, K32("create"));

	Array <Variable> prompted_inputs;

	auto select = [&args, &std_out, std_in, prefs](CString::View id, bool multi, ArrayView <CString> valid) -> CString
	{
		Array <CString> rtn;

		if (auto property = Data::GetCString(args, id))
		{
			auto parts = Split(property, ',');

			for (auto & i : parts)
			{
				rtn.Push(Lowercase(i));
			}
		}
		else
		{
			File::WriteLine(std_out, multi ? Join(id, "(s):") : Join(id, ':'));

			REFLEX_LOOP(idx, valid.size)
			{
				CLI::Print(std_out, Join(kColourDim, ToCString(idx + 1), ": ", valid[idx], kColourDefault));
			}

			auto prompt = PromptValue(std_in, std_out, Data::GetWString(prefs, id));

			auto parts = Split(prompt, ',');

			for (auto & i : parts)
			{
				i = Trim(i);

				if (i)
				{
					if (Data::Detail::kChar2Type[char(i.GetFirst())] == Data::Detail::kCharTypeNumber)
					{
						auto idx = ToUInt32(i);

						if (idx > 0 && idx <= valid.size)
						{
							rtn.Push(valid[idx - 1]);
						}
					}
					else
					{
						rtn.Push(ToCString(i));
					}
				}
			}
		}

		if (rtn.Empty()) goto Fail;

		if (!multi && rtn.GetSize() != 1) goto Fail;

		for (auto & i : rtn)
		{
			if (!Search(valid, i)) goto Fail;
		}

		return Merge(rtn, ',');

		Fail:
		CLI::ThrowError(Join("invalid --", id));
		return {};
	};

	auto templates = GetTemplates();

	Array <CString> template_ids, target_ids;

	for (auto & i : templates) template_ids.Push(GetTemplateID(i));
	for (auto & i : kTargets) target_ids.Push(i);

	if (auto default_target = get_default_target())
	{
		Remove(target_ids, default_target);
		target_ids.Insert(1, default_target);
	}

	auto template_from_args = True(Data::GetCString(args, "template"));
	auto target_from_args = True(Data::GetCString(args, "target"));

	CString template_arg = select("template", false, template_ids);
	CString target_arg = select("target", true, target_ids);

	if (!template_from_args) prompted_inputs.Push({ "template", ToWString(template_arg) });
	if (!target_from_args) prompted_inputs.Push({ "target", ToWString(target_arg) });

	auto ptmpl = SearchTemplate(templates, template_arg);

	auto targets = FindTargets(target_arg);

	Tuple <bool, const Array <TokenDefinition> &, Array <Variable>> groups[] =
	{
		{ false, ptmpl->strings },
		{ true, ptmpl->paths }
	};

	for (auto & group : groups)
	{
		for (auto & token : group.b)
		{
			File::WriteLine(std_out, Join(token.id, ':'));

			WString value;

			if (group.a)
			{
				if (auto arg = CLI::GetFolder(args, token.id, false))
				{
					value = arg;
				}
				else
				{
					value = prompt_required(Data::GetWString(prefs, token.id));

					value = File::CorrectStrokes(value);

					prompted_inputs.Push({ Join(token.id), value });
				}
			}
			else
			{
				if (auto arg = Data::GetCString(args, token.id))
				{
					value = ToWString(arg);
				}
				else
				{
					value = prompt_required(Data::GetWString(prefs, token.id));
					
					prompted_inputs.Push({ Join(token.id), value });
				}
			}

			group.c.Push({ token.token, value });
		}
	}

	auto output = CLI::GetFolder(args, "output", false);
	
	if (output.Empty())
	{
		File::WriteLine(std_out, Join("output", ':'));

		output = prompt_required(System::GetCurrentDirectory());

		output = File::CorrectStrokes(output);
	}

	if (CaseInsensitive::eq(Left<true>(output, reflex_path.GetSize()), reflex_path)) CLI::ThrowError("invalid output path, pass --output <folder> to a location outside the reflex repository");

	auto folder = CreateProject(*ptmpl, groups[0].c, groups[1].c, targets, output, CLI::GetBool(args, "overwrite"), std_out);

	for (auto & input : prompted_inputs)
	{
		Data::SetWString(prefs, input.token, input.value);
	}

	File::WriteLine(std_out, Join(L"project created at ", folder));
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
				print_overview_command_summary("templates", "list available Reflex project templates");
				print_overview_command_summary("targets", "list available project generation targets");

				File::WriteLine(std_out);
				CLI::Print(std_out, CLI::kColourBrightBlack, "SDK Install:");
				print_overview_command_summary("install", "install or update the SDK");
				print_overview_command_summary("version", "show the installed SDK version");
				print_overview_command_summary("versions", "list available SDK versions");
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
					print_arg(true, "--overwrite false", "allow overwriting source files");
					return;

				case K32("install"):
					print_arg(false, "[version]", "the requested SDK version to install, leave unspecified for latest");
					print_arg(true, "--platforms <win|macos|android|ios[,..]>", "the platform packages to install");
					print_arg(true, "--path <folder>", "install the SDK to a specific location");
					print_arg(true, "--test true", "download and extract packages without moving files into place");
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
				case K32("versions"):
				case K32("templates"):
				case K32("targets"):
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
		.id = K32("where"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			File::WriteLine(std_out, GetReflexPath());
		}
	},
	{
		.id = K32("templates"),
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
		.id = K32("targets"),
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
			Install(Data::GetCString(args, "value"), CLI::GetStringArray(args, "platforms"), CLI::GetFolder(args, "path", false), CLI::GetBool(args, "test"), std_out);
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
		.id = K32("versions"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			ListVersions(std_out);
		}
	},
	{
		.id = K32("create"),
		.fn = &Create
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
	{
		.id = K32("reset"),
		.fn = [](const Data::PropertySet & args, System::FileHandle & std_out)
		{
			Data::ResetPropertySet(Data::kPropertySetFormat, Bootstrap::global->prefs);
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
