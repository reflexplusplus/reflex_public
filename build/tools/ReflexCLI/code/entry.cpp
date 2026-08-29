#include "tasks.h"
#include "reflex_ext/bootstrap/console_app.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr Key32 kTasks = "tasks";
constexpr auto kFirstPositionalArg = K32("value");

CString GetTemplateID(const TemplateDefinition & tmpl)
{
	return ToCString(File::SplitFilename(File::RemoveTrailingStroke(tmpl.folder)).b);
}

WString PromptValue(System::FileHandle & std_in, System::FileHandle & std_out, WString::View default_value = {}, WString::View default_label = {})
{
	CString prompt;
	
	if (default_value) prompt = Join(kColourDim, "[", ToCString(default_label ? default_label : default_value), "]: ", kColourDefault);

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

bool PromptBool(System::FileHandle & std_in, System::FileHandle & std_out, CString::View title, bool default_value)
{
	File::WriteLine(std_out, Join(kColourDefault, title, kColourDim, '?', kColourDefault));

	WString::View choices[] = { L"n", L"y" };
	WString prompt = L"n/y";
	prompt[default_value ? 2 : 0] = Uppercase(prompt[default_value ? 2 : 0]);

	auto value = PromptValue(std_in, std_out, choices[default_value], prompt);
	
	return CaseInsensitive::eq(value, choices[true]);
}

Array <CString::View> FindTargets(const CString::View & arg)
{
	Array <CString::View> targets;

	for (auto & raw : Split(arg, ','))
	{
		auto value = Trim(raw);

		if (auto ptarget = SearchValue<StringCompare>(ToView(kBuildPlatforms), value))
		{
			if (!Search(targets, *ptarget)) targets.Push(*ptarget);
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
	auto reflex_path = GetReflexPath();

	auto std_in = Make<System::FileHandle>(System::FileHandle::kStandardStreamIn);

	const auto prompt_required = [&std_out, std_in](CString::View title, WString::View default_value = {}) -> WString
	{
		File::WriteLine(std_out, Join(title, ':'));

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
	Array <Pair<CString>> string_inputs;

	auto select = [&args, &std_out, std_in, prefs](CString::View id, bool multi, ArrayView <CString> valid, bool allow_all = false) -> CString
	{
		Array <CString> rtn;

		if (auto property = Data::GetCString(args, id))
		{
			if (allow_all && CaseInsensitive::eq(Trim(property), "all")) return Merge(valid, ',');

			auto parts = Split(property, ',');

			for (auto & i : parts)
			{
				rtn.Push(Lowercase(i));
			}
		}
		else
		{
			File::WriteLine(std_out, Join(id, ':'));

			REFLEX_LOOP(idx, valid.size)
			{
				CLI::Print(std_out, Join(kColourDim, ToCString(idx + 1), ": ", valid[idx], kColourDefault));
			}

			auto default_value = Data::GetWString(prefs, id);
			if (allow_all && default_value.Empty()) default_value = L"all";

			auto prompt = PromptValue(std_in, std_out, default_value);

			if (allow_all && CaseInsensitive::eq(prompt, L"all")) return Merge(valid, ',');

			auto parts = Split(prompt, ',');

			for (auto & i : parts)
			{
				i = Trim(i);

				if (i)
				{
					if (Data::Detail::CharToType(char(i.GetFirst())) == Data::Detail::kCharTypeNumber)
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

	Array <CString> template_ids;

	for (auto & i : templates) template_ids.Push(GetTemplateID(i));

	auto template_from_args = True(Data::GetCString(args, "template"));

	CString template_arg = select("template", false, template_ids);

	if (!template_from_args) prompted_inputs.Push({ "template", ToWString(template_arg) });

	auto ptmpl = SearchTemplate(templates, template_arg);

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
				if (auto arg = CLI::GetFolder(args, token.id, false))
				{
					value = arg;
				}
				else
				{
					value = prompt_required(token.id, Data::GetWString(prefs, token.id));

					value = CLI::Detail::ExpandPath(token.id, value, true, false);

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
					value = prompt_required(token.id, Data::GetWString(prefs, token.id));

					prompted_inputs.Push({ Join(token.id), value });
				}
			}

			if (!group.a) string_inputs.Push({ token.id, ToCString(value) });

			group.c.Push({ token.token, value });
		}
	}

	auto output = CLI::GetFolder(args, "output", false);

	if (output.Empty())
	{
		auto current_dir = System::GetCurrentDirectory();

		output = prompt_required("output", current_dir);

		output = File::CorrectStrokes(output);

		if (output.GetFirst() != L'~' && !System::IsAbsolutePath(output))
		{
			output = Join(current_dir, output);
		}

		output = CLI::Detail::ExpandPath("output", output, true, false);
	}

	output = File::CorrectTrailingStroke(output);

	//check not in REFLEX_PATH path, except REFLEX_PATH/temp/...

	if (CaseInsensitive::eq(Left<true>(output, reflex_path.GetSize()), reflex_path))
	{
		if (!CaseInsensitive::eq(Mid<true>(output, reflex_path.GetSize(), 4), ToView(L"tmp/")))
		{
			CLI::ThrowError("invalid output path, pass --output <folder> to a location outside the reflex repository");
		}
	}

	Require(True(ptmpl->platforms), "template platforms", "undefined");

	auto generate_from_args = True(Data::GetCString(args, "generate"));
	auto generate_arg = select("generate", true, ptmpl->platforms, true);
	auto targets = FindTargets(generate_arg);

	if (!generate_from_args) prompted_inputs.Push({ "generate", ToWString(generate_arg) });

	Optional <bool> overwrite(CLI::GetBool(args, "overwrite"), prompted_inputs.Empty());

	CreateProject(*ptmpl, groups[0].c, groups[1].c, targets, output, std_in, std_out, [&std_in, &std_out, &overwrite](const WString & path)
	{
		if (!overwrite.set)
		{
			overwrite = PromptBool(std_in, std_out, "Overwrite files (cfg,h,cpp,c,glx)", false);
		}
		return overwrite.value;
	});

	for (auto & input : prompted_inputs) Data::SetWString(prefs, input.name, input.value);
}

void RunBuildHelpers(ArrayView<WString> scripts, System::FileHandle & std_out, CString::View action)
{
	const bool is_windows = System::kPlatform == System::kPlatformWindows;
	WString::View tool = is_windows ? L"cmd.exe" : L"/bin/sh";
	bool ran = false;

	for (auto & i : scripts)
	{
		if (File::Exists(i))
		{
			Array <WString> commands;

			if (is_windows) commands.Append({ L"/d", L"/c" });

			commands.Push(i);

			Require(RunCommand(tool, commands, &std_out, true), Join(action, " failed"), ToCString(i));

			ran = true;
		}
	}

	Require(ran, Join(action, " failed"), Join("nothing to ", action));
}

WString GetProjectFolder(const Data::PropertySet & args)
{
	if (Data::GetCString(args, "path")) return CLI::GetFolder(args, "path", true);

	return System::GetCurrentDirectory();
}

void Build(const Data::PropertySet & args, System::FileHandle & std_out)
{
	static constexpr WString::View kPrefix = L"Build ";

	const auto ext = System::kPlatform == System::kPlatformWindows ? kBat : kCommand;
	auto platform = Data::GetCString(args, kFirstPositionalArg);
	Require(True(Search(kBuildPlatforms, platform)), "expected platform", "<windows|macos|linux|ios|android> [configuration]");
	auto directory = Join(GetProjectFolder(args), L"projects/", ToWString(platform), File::kStroke);
	auto make_config = [ext](WString::View configuration) { return Join(kPrefix, configuration, File::kDot, ext); };

	Array<WString> scripts;
	if (auto configuration = Data::GetCString(args, kFirstPositionalArg + 1))
	{
		scripts.Push(Join(directory, make_config(ToWString(configuration))));
	}
	else
	{
		auto [folders, files] = File::List(directory, false);
		for (auto & [name, unused] : files)
		{
			if (CaseInsensitive::eq(Left<true>(name, kPrefix.size), kPrefix) && File::CheckExtension(name, ext)) scripts.Push(Join(directory, name));
		}
	}

	RunBuildHelpers(scripts, std_out, "build");
}

void Clean(const Data::PropertySet & args, System::FileHandle & std_out)
{
	const auto ext = System::kPlatform == System::kPlatformWindows ? kBat : kCommand;
	Array<CString::View> platforms;
	for (UInt index = 0; auto platform = Data::GetCString(args, kFirstPositionalArg + index); ++index)
	{
		Require(True(Search(Left(kBuildPlatforms, kBuildPlatformCMake), platform)), "expected platform", "<windows|macos|ios|android|linux>...");
		platforms.Push(platform);
	}
	if (platforms.Empty()) platforms.Append(Left(kBuildPlatforms, kBuildPlatformCMake));

	auto project_folder = GetProjectFolder(args);
	Array<WString> scripts;
	for (auto platform : platforms) scripts.Push(Join(project_folder, L"projects/", ToWString(platform), File::kStroke, L"Clean.", ext));

	RunBuildHelpers(scripts, std_out, "clean");
}

void Run(const Data::PropertySet & args, System::FileHandle & std_out)
{
	auto command = Data::GetCString(args, kFirstPositionalArg);
	Require(True(command), "expected command", "<command> [args...]");

	Array<WString> command_args;
	for (UInt index = 1; auto arg = Data::GetCString(args, kFirstPositionalArg + index); ++index) command_args.Push(ToWString(arg));
	Require(RunCommand(ToWString(command), command_args, &std_out), "run failed", command);
}

void Edit(const Data::PropertySet & args)
{
	auto platform = Data::GetCString(args, kFirstPositionalArg);
	Require(True(platform), "expected platform", "<windows|macos|ios|android>");

	auto directory = Join(GetProjectFolder(args), L"projects/", ToWString(platform), File::kStroke);
	Require(File::Exists(directory), "edit failed", "platform has not been generated");

	auto find_project = [&directory](WString::View extension, bool is_folder)
	{
		auto [folders,files] = File::List(directory, false);
		auto & entries = is_folder ? folders : files;
		WString result;
		for (auto & [name, unused] : entries)
		{
			if (File::CheckExtension(File::RemoveTrailingStroke(name), extension))
			{
				Require(result.Empty(), "edit failed", "multiple IDE projects found");
				result = Join(directory, name);
			}
		}
		Require(True(result), "edit failed", "IDE project was not found");
		return result;
	};

	switch (MakeKey32(platform))
	{
	case K32("windows"):
		Require(System::Open(find_project(L"sln", false)), "edit failed", "could not open Visual Studio");
		return;

	case K32("macos"):
	case K32("ios"):
		Require(System::Open(find_project(L"xcworkspace", true)), "edit failed", "could not open Xcode");
		return;

	case K32("android"):
	{
#if defined(REFLEX_OS_WINDOWS)
		auto studio = WString(L"C:/Program Files/Android/Android Studio/bin/studio64.exe");
		Require(File::Exists(studio) && RunCommand(L"cmd.exe", { L"/d", L"/c", L"start", L"", studio, directory }, nullptr, true), "edit failed", "could not open Android Studio");
#elif defined(REFLEX_OS_MACOS)
		Require(RunCommand(L"/usr/bin/open", { L"-a", L"Android Studio", directory }), "edit failed", "could not open Android Studio");
#else
		ThrowError("edit failed", "Android Studio is not supported on this host");
#endif
		return;
	}

	case K32("linux"):
	case K32("cmake"):
		ThrowError("edit failed", "this platform does not define an IDE project");
		return;

	default:
		ThrowError("expected platform", "<windows|macos|ios|android|linux|cmake>");
		return;
	}
}

consteval CLI::TaskDef MakeTask(Key32 id, CLI::TaskFn fn) { return { id, fn }; }

void UnsetPersistentVariable(Data::PropertySet & variables, Key32 id)
{
	Data::UnsetBool(variables, id);
	Data::UnsetKey32(variables, id);
	Data::UnsetCString(variables, id);
}

void ExportState(const Data::PropertySet & args, System::FileHandle & std_out)
{
	auto path = CLI::GetFilename(args, "path", false, "export.cfg");
	if (File::Exists(path))
	{
		auto std_in = Make<System::FileHandle>(System::FileHandle::kStandardIn);
		if (!PromptBool(*std_in, std_out, "Replace existing export.cfg", false)) return;
	}

	Data::PropertySet root;
	auto root_keymap = Data::AcquireKeyMap(root);
	Data::RegisterKey(root_keymap, "tasks");
	Data::RegisterKey(root_keymap, "variables");
	Data::RegisterKey(root_keymap, "id");
	Data::RegisterKey(root_keymap, "directory");
	Data::RegisterKey(root_keymap, "commands");
	Data::RegisterKey(root_keymap, "arguments");

	for (auto i : { kTasks, kPersistentVariables })
	{
		if (auto tasks = Data::GetPropertySet(Bootstrap::global->prefs, i))
		{
			Data::Assimilate(root_keymap, Data::GetKeyMap(tasks));
			Data::SetPropertySet(root, i, New<Data::PropertySet>(*tasks));
		}
	}

	Require(File::Save(path, Data::EncodePropertySet(Data::kPropertySheetFormat, root)), "export failed", path);
}

void ImportState(const Data::PropertySet & args)
{
	auto path = CLI::GetFilename(args, "path", true, "export.cfg");
	auto imported = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(path));
	if (auto error = Data::GetError(imported)) ThrowError(error.value.c, ToCString(error.value.a));
	auto keymap = Data::GetKeyMap(imported);
	if (auto error = Data::GetError(imported)) ThrowError(ToCString(error.value.a), error.value.c);

	if (auto variables = Data::GetPropertySet(imported, kPersistentVariables))
	{
		auto persistent = Data::AcquirePropertySet(Bootstrap::global->prefs, kPersistentVariables);
		Data::Assimilate(Data::AcquireKeyMap(persistent), keymap);
		for (auto & [address, value] : variables->Iterate())
		{
			if (address.type_id == GetTypeID<Data::KeyMap>()) continue;
			auto name = Data::GetKey(keymap, address.id);
			Require(IsVariableName(name), "invalid variable", name);
			UnsetPersistentVariable(persistent, name);
			auto is_string = address.type_id == GetTypeID<Data::CStringProperty>();
			if (is_string || address.type_id == GetTypeID<Data::BoolProperty>())
			{
				if (is_string) Data::SetKey32(persistent, name, Cast<Data::CStringProperty>(value)->value);

				persistent->SetProperty(address, value);
			}
			else
			{
				ThrowError("invalid variable", name);
			}
		}
	}

	if (auto tasks = Data::GetPropertySet(imported, kTasks))
	{
		auto persistent = Data::AcquirePropertySet(Bootstrap::global->prefs, kTasks);
		Data::Assimilate(Data::AcquireKeyMap(persistent), keymap);
		for (auto & [address, task] : tasks->Iterate<Data::PropertySet>())
		{
			auto id = Data::GetKey(keymap, address.id);
			auto directory = Data::GetWString(task, "directory");
			auto commands = Data::GetCStringArray(task, "commands");
			Require(True(id) && directory && commands.size, "invalid task", "tasks require id, directory, and commands");

			Array<CString> arguments;
			for (auto & argument : GetCStrings(task, "arguments"))
			{
				Require(IsVariableName(argument) && !Search(arguments, argument), "invalid task argument", argument);
				arguments.Push(argument);
			}
		}
		Data::Assimilate(*persistent, tasks);
	}
}

const CLI::TaskDef kCommands[] =
{
	MakeTask("help", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		const auto print_arg = [&std_out](bool optional, CString::View name, CString::View description)
		{
			const auto kColourWhite = CLI::Detail::kColours[CLI::kColourWhite];
			const auto kColourName = optional ? kColourDim : kColourWhite;
			const auto kColourText = optional ? kColourDim : kColourDefault;

			File::WriteLine(std_out, Join(kColourName, name, ' ', kColourText, description, kColourDefault));
		};

		const auto show_overview = [&]()
		{
			PrintCommandWithDescription(std_out, "doc", "browse documentation");

			File::WriteLine(std_out);
			PrintCommandWithDescription(std_out, "create", "create a new project from a template");
			PrintCommandWithDescription(std_out, "templates", "list available project templates");

			File::WriteLine(std_out);
			PrintCommandWithDescription(std_out, "generate", "generate native projects from project.cfg");
			PrintCommandWithDescription(std_out, "open", "open a project folder in the system file manager");
			PrintCommandWithDescription(std_out, "edit", "open a generated IDE project");
			PrintCommandWithDescription(std_out, "clean", "clean generated build output");
			PrintCommandWithDescription(std_out, "build", "run generated native build helpers");
			PrintCommandWithDescription(std_out, "set", "set a build variable");
			PrintCommandWithDescription(std_out, "get", "view a build variable");
			PrintCommandWithDescription(std_out, "unset", "remove a build variable");
			PrintCommandWithDescription(std_out, "variables", "list build variables");
			PrintCommandWithDescription(std_out, "export", "export variables");
			PrintCommandWithDescription(std_out, "import", "import variables");
			PrintCommandWithDescription(std_out, "build-resources", "build Reflex++ resource output for a source file");
			PrintCommandWithDescription(std_out, "build-plist", "generate an Info.plist file for a supported target");

			File::WriteLine(std_out);
			PrintCommandWithDescription(std_out, "install", "install or update the SDK");
			PrintCommandWithDescription(std_out, "version", "show the installed SDK version");
			PrintCommandWithDescription(std_out, "versions", "list available SDK versions");
			PrintCommandWithDescription(std_out, "where", "show the install location");
			PrintCommandWithDescription(std_out, "set-path", "select the SDK installation used by new terminal sessions");

			//custom task api not documented
			//PrintCommandWithDescription(std_out, "run", "run a command with arguments");

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
				print_arg(true, "--generate <list>", "the platform project(s) to generate, defaults to all supported");
				print_arg(true, "--overwrite false", "allow overwriting source files");
				return;

			case K32("generate"):
				print_arg(true, "--path <path>", "the project.cfg description to generate, defaults to ./project.cfg");
				print_arg(true, "[platform]...", "platforms to generate, defaults to all platforms in project.cfg");
				return;

			case K32("build"):
				print_arg(false, "<platform>", "the generated platform folder, for example windows");
				print_arg(true, "[configuration]", "run only this configuration, for example debug");
				print_arg(true, "--path <folder>", "the project folder, defaults to the working folder");
				return;

			case K32("clean"):
				print_arg(true, "[platform]...", "clean all configurations, defaults to every generated platform");
				print_arg(true, "--path <folder>", "the project folder, defaults to the working folder");
				return;

			case K32("run"):
				print_arg(false, "<command> [args...]", "run a command with arguments");
				return;

			case K32("install"):
				print_arg(false, "[version]", "the requested SDK version to install, leave unspecified for latest");
				print_arg(true, "--platforms <win|macos|android|ios[,..]>", "the platform packages to install");
				print_arg(true, "--path <folder>", "install the SDK to a specific location");
				print_arg(true, "--test true", "download and extract packages without moving files into place");
				return;

			case K32("doc"):
				DocHelp(std_out);
				return;

			case K32("build-resources"):
				print_arg(true, "--path <path>", "the resources.xml description to build, defaults to ./resources.xml");
				return;

			case K32("build-plist"):
				print_arg(false, "--target <app|audioapp|ios_app|ios_audioapp|vst2|vst3|clap|au|auv3>", "the plist target type to generate");
				print_arg(false, "--output <path>", "the output plist file path");
				print_arg(false, "--product <name>", "the product name to embed");
				print_arg(false, "--bundle_id <id>", "the bundle identifier");
				print_arg(false, "--version <x.y.z>", "the product version string");
				print_arg(true, "--app_store_category <id>", "the App Store category identifier");
				print_arg(true, "--vendor <name>", "the vendor name to embed");
				print_arg(true, "--au_components <id:type[:name],...>", "Audio Unit component records (4CC subtype and type)");
				print_arg(true, "--au_manufacturer <4cc>", "the Audio Unit manufacturer code");
				return;

			case K32("version"):
			case K32("versions"):
			case K32("templates"):
			case K32("where"):
				print_arg(true, "no arguments", "");
				return;

			case K32("open"):
				print_arg(true, "--path <folder>", "the project folder, defaults to the working folder");
				return;

			case K32("edit"):
				print_arg(false, "<windows|macos|ios|android>", "the generated IDE project to open");
				print_arg(true, "--path <folder>", "the project folder, defaults to the working folder");
				return;

			case K32("set-path"):
				print_arg(false, "<folder>", "the Reflex++ repository to select");
				return;

			case K32("set"):
				print_arg(false, "<name> <value>", "set a string variable; true and false are stored as booleans");
				return;

			case K32("get"):
				print_arg(false, "<name>", "view a persistent project variable");
				return;

			case K32("unset"):
				print_arg(false, "<name>", "remove a persistent project variable");
				return;

			case K32("variables"):
				print_arg(true, "no arguments", "list persistent project variables");
				return;

			case K32("export"):
				print_arg(true, "--path <file>", "the export file path, defaults to ./export.cfg");
				return;

			case K32("import"):
				print_arg(true, "--path <file>", "the import file path, defaults to ./export.cfg");
				return;

			default:
				show_overview();
				return;
			}
		};

		if (auto command = Data::GetCString(args, kFirstPositionalArg))
		{
			show_command_help(command);
		}
		else
		{
			show_overview();
		}
	}),
	MakeTask("where", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		File::WriteLine(std_out, GetReflexPath());
	}),
	MakeTask("open", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Require(System::Open(GetProjectFolder(args)), "open failed", "could not open the project folder");
	}),
	MakeTask("edit", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Edit(args);
	}),
	MakeTask("set-path", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		SetPath(CLI::GetFolder(args, "value", true), std_out);
	}),
	MakeTask("templates", [](const Data::PropertySet & args, System::FileHandle & std_out)
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
			Data::RegisterKey(keymap, "platforms");
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
	}),
	MakeTask("install", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Install(Data::GetCString(args, kFirstPositionalArg), CLI::GetStringArray(args, "platforms"), CLI::GetFolder(args, "path", false), CLI::GetBool(args, "test"), std_out);
	}),
	MakeTask("version", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		GetVersion(std_out);
	}),
	MakeTask("versions", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		ListVersions(std_out);
	}),
	MakeTask("doc", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Doc(args, std_out);
	}),
	MakeTask("create", &Create),
	MakeTask("generate", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Array <CString::View> platforms;
		auto key = kFirstPositionalArg;
		while (auto arg = Data::GetCString(args, key++))
		{
			Require(True(Search(kBuildPlatforms, arg)), "expected platform", "<windows|macos|ios|android|linux|cmake>...");
			platforms.Push(arg);
		}
		if (platforms.Empty()) platforms = Left(kBuildPlatforms, kBuildPlatformCMake);

		GenerateProject(CLI::GetFilename(args, "path", true, "project.cfg"), platforms, std_out);
	}),
	MakeTask("build", &Build),
	MakeTask("clean", &Clean),
	MakeTask("run", &Run),
	MakeTask("build-resources", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Float progress = 0.0f;

		BuildResources(CLI::GetFilename(args, "path", true, "resources.xml"), progress);
	}),
	MakeTask("build-plist", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		BuildPlist(args, std_out);
	}),
	MakeTask("set", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		auto name = Data::GetCString(args, kFirstPositionalArg);
		auto value = args.QueryProperty<Data::CStringProperty>(kFirstPositionalArg + 1);
		Require(IsVariableName(name) && value, "expected variable", "<name> <value>");
		auto variables = Data::AcquirePropertySet(Bootstrap::global->prefs, kPersistentVariables);
		auto keymap = Data::AcquireKeyMap(*variables);
		auto id = Data::RegisterKey(keymap, name);
		UnsetPersistentVariable(variables, id);
		if (auto idx = Search(ToView(Reflex::Detail::kFalseTrue), value->value))
		{
			Data::SetBool(*variables, id, idx.value == 1);
		}
		else
		{
			Data::SetCString(*variables, id, value->value);
			Data::SetKey32(*variables, id, Data::RegisterKey(keymap, value->value));
		}
	}),
	MakeTask("get", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		auto name = Data::GetCString(args, kFirstPositionalArg);
		auto variables = GetPersistentVariables();
		if (auto variable = FindVariable(variables, name)) File::WriteLine(std_out, EncodeUTF8(variable->value));
	}),
	MakeTask("unset", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		auto name = Data::GetCString(args, kFirstPositionalArg);
		UnsetPersistentVariable(Data::AcquirePropertySet(Bootstrap::global->prefs, kPersistentVariables), name);
	}),
	MakeTask("variables", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		auto variables = GetPersistentVariables();
		Sort(variables, [](const Variable & a, const Variable & b) { return a.name < b.name; });
		for (auto & variable : variables)
		{
			File::WriteLine(std_out, Join(variable.name, '=', EncodeUTF8(variable.value)));
		}
	}),
	MakeTask("export", &ExportState),
	MakeTask("import", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		ImportState(args);
	}),
	MakeTask("reset", [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		Data::ResetPropertySet(Data::kPropertySetFormat, Bootstrap::global->prefs);
	}),
	MakeTask(kTasks, [](const Data::PropertySet & args, System::FileHandle & std_out)
	{
		auto tasks = Data::GetPropertySet(Bootstrap::global->prefs, kTasks);

		for (auto [address, task] : tasks->Iterate<Data::PropertySet>())
		{
			auto id = Data::GetKey(Data::GetKeyMap(tasks), address.id);
			auto commands = Data::GetCStringArray(task, "commands");
			auto directory = Data::GetWString(task, "directory");
			CString signature(id);
			for (auto & argument : GetCStrings(task, "arguments")) signature.Append(Join(" <", argument, ">"));
			File::WriteLine(std_out, Join(signature, kColourDim, " = ", Merge(commands, ' '), " @ ", EncodeUTF8(directory), kColourDefault));
		}
	}),
};

struct TaskRunner
{
	TaskRunner(const Data::PropertySet & tasks, System::FileHandle & out)
		: m_tasks(tasks)
		, m_out(out)
	{
	}

	bool Run(CString::View task_name, const Data::PropertySet & inputs)
	{
		auto id = MakeKey32(task_name);
		auto task = Data::GetPropertySet(m_tasks, id);
		Require(True(task), "unknown task", task_name);
		Require(!Search(m_active, id), "task cycle", task_name);

		Array<Variable> variables = m_variables ? m_variables.GetLast() : GetPersistentVariables();
		auto arguments = GetCStrings(task, "arguments");
		Array<CString> argument_names;
		REFLEX_LOOP(index, arguments.size)
		{
			auto & name = arguments[index];
			Require(IsVariableName(name) && !Search(argument_names, name), "invalid task argument", name);
			argument_names.Push(name);

			auto value = Data::GetCString(inputs, name);
			if (!value) value = Data::GetCString(inputs, kFirstPositionalArg + index);
			if (value) SetVariable(variables, name, ToWString(value));
			Require(FindVariable(variables, name), Join(task_name, ": missing argument"), name);
		}
		Require(!Data::GetCString(inputs, kFirstPositionalArg + arguments.size), "too many task arguments", task_name);

		auto directory = EvaluateVariableExpressions(Data::GetWString(task, "directory"), variables, kVariableSyntaxProject, {});
		Scope scope(m_active, m_variables, id, std::move(variables));
		Require(System::SetCurrentDirectory(directory), "task failed", "failed to set current directory");

		auto commands = Data::GetCStringArray(task, "commands");
		while (commands)
		{
			auto line = commands;
			if (auto pos = Search(commands, "+"))
			{
				commands = Nudge(commands, pos.value + 1);
				line.size = pos.value;
			}
			else
			{
				commands = {};
			}

			Array<CString> expanded;
			for (auto & argument : line)
			{
				expanded.Push(EncodeUTF8(EvaluateVariableExpressions(ToWString(argument), m_variables.GetLast(), kVariableSyntaxProject, {}, System::kPlatform)));
			}
			auto args = MakeArray(expanded, [](const CString & arg) { return ToView(arg); });
			if (Bootstrap::CLI::Detail::Dispatch(args, ToView(kCommands), Bootstrap::CLI::kFlagPrintError, m_out, this, &Fallback)) return false;
		}

		return true;
	}

private:

	struct Scope
	{
		Scope(Array<Key32> & active, Array<Array<Variable>> & variables, Key32 task, Array<Variable> task_variables)
			: active(active)
			, variables(variables)
			, previous(System::GetCurrentDirectory())
		{
			active.Push(task);
			variables.Push(std::move(task_variables));
		}

		~Scope()
		{
			variables.Pop();
			active.Pop();
			System::SetCurrentDirectory(previous);
		}

		Array<Key32> & active;
		Array<Array<Variable>> & variables;
		WString previous;
	};

	static bool Fallback(void * client, ArrayView<CString::View> cmdline, Key32, const Data::PropertySet & args, System::FileHandle &)
	{
		return Cast<TaskRunner>(client)->Run(cmdline[0], args);
	}

	const Data::PropertySet & m_tasks;
	System::FileHandle & m_out;
	Array<Key32> m_active;
	Array<Array<Variable>> m_variables;
};

REFLEX_END_INTERNAL

Reflex::Output ReflexCLI::output("Reflex");

Reflex::UInt8 Reflex::System::OnStart(const ArrayView <CString::View> & cmdline)
{
#if REFLEX_DEBUG
	auto agent_args = Bootstrap::ParseCmdlineArgs(cmdline, true);
	if (Data::GetBool(agent_args, "terminate-on-assert"))
	{
		System::Detail::DebugBreak = [](const char * msg)
		{
			auto file = Make<System::FileHandle>(System::FileHandle::kStandardStreamOut);
			File::WriteLine(*file, "*** REFLEX_ASSERT ***");
			File::WriteLine(*file, msg);
			System::Detail::EnumerateStackTrace(file.Adr(), [](void * pfile, UInt frame, const void * address, const char * symbol)
			{
				auto file = Cast<System::FileHandle>(pfile);
				File::WriteLine(*file, Reflex::Detail::DebugJoin(" ", frame, address, symbol));
			});
			file->Flush(true);
			System::Detail::Terminate(1);
		};
	}
#endif

#if REFLEX_DEBUG
	REFLEX_LOCAL(void, EnumerateResources)(const WString & root, const WString & folder, Array <CString> &paths)
	{
		auto [folders, files] = File::List(Join(root, folder));
		for (auto & file : files) paths.Push(ToCString(Join(folder, file.key)));
		for (auto & child : folders) Call(root, Join(folder, child.key), paths);
	}
	REFLEX_END

	auto resources = Join(Bootstrap::Detail::ExtractProjectDir(__FILE__), L"resources/project_generator", File::kStroke);
	Array <CString> paths;
	EnumerateResources::Call(Join(resources, L"android", File::kStroke), {}, paths);
	Sort(paths, [](const CString & a, const CString & b) { return a < b; });

	Data::Archive listing;
	for (auto & path : paths) Data::WriteLine(listing, path);
	listing.Pop();
	File::Save(Join(resources, L"android_listing.txt"), listing);
#endif

	auto global = AutoRelease(Bootstrap::Global::Acquire("Reflex++", "ReflexCLI", Bootstrap::Detail::ExtractProjectDir(__FILE__)));

	auto out = Make<System::FileHandle>(System::FileHandle::kStandardOut);

	return Bootstrap::CLI::Detail::Dispatch(cmdline, ToView(ReflexCLI::kCommands), Bootstrap::CLI::kFlagPrintError, out, nullptr, [](void * client, ArrayView <CString::View> cmdline, Key32 task, const Data::PropertySet & args, System::FileHandle & out)
	{
		if (cmdline.size > 2 && cmdline[1] == "=")
		{
			auto id = cmdline[0];
			auto tasks = Data::AcquirePropertySet(Bootstrap::global->prefs, ReflexCLI::kTasks);
			Array<CString> commands;
			for (auto arg : Mid<true>(cmdline, 2)) commands.Push(arg);

			Data::RegisterKey(Data::AcquireKeyMap(*tasks), id);
			auto task = Data::AcquirePropertySet(tasks, id);
			Data::SetWString(task, "directory", System::GetCurrentDirectory());
			Data::SetCStringArray(task, "commands", commands);

			return true;
		}
		else
		{
			return ReflexCLI::TaskRunner(Data::GetPropertySet(Bootstrap::global->prefs, "tasks"), out).Run(cmdline[0], args);
		}
	});
}
