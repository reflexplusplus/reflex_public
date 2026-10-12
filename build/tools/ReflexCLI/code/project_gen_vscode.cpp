#include "project_gen_emitter.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen::VSCode)

CString Json(CString::View value)
{
	constexpr char digits[] = "0123456789abcdef";
	CString result = "\"";
	for (auto c : value)
	{
		if (c == '"' || c == '\\') { result.Push('\\'); result.Push(c); }
		else if (UInt8(c) < 32)
		{
			result.Append("\\u00");
			result.Push(digits[UInt8(c) >> 4]);
			result.Push(digits[UInt8(c) & 15]);
		}
		else result.Push(c);
	}
	result.Push('"');
	return result;
}

CString JsonArray(ArrayView<CString> values)
{
	Array<CString> quoted;
	for (auto & value : values) quoted.Push(Json(value));
	return Join('[', Merge(quoted, ','), ']');
}

CString ShellCommand(ArrayView<CString> arguments)
{
	Array<CString> quoted;
	for (auto & argument : arguments)
	{
		if (System::kPlatform == System::kPlatformWindows)
			quoted.Push(Join("'", Replace(argument, "'", "''"), "'"));
		else quoted.Push(ShellQuote(argument));
	}
	return Join(System::kPlatform == System::kPlatformWindows ? "& " : "", Merge(quoted, ' '));
}

bool Supports(BuildPlatform platform)
{
	if (platform == kBuildPlatformAndroid || platform == kBuildPlatformWeb) return true;
	if (platform == kBuildPlatformIOS) return System::kPlatform == System::kPlatformMacOS;
	return UInt(platform) == UInt(System::kPlatform);
}

CString ArchitectureName(BuildPlatform platform, Architecture architecture)
{
	if (platform == kBuildPlatformWindows && architecture == kArchitecture_x86) return "Win32";
	if (platform == kBuildPlatformMacOS && architecture == kArchitecture_x64) return "x86_64";
	return CString(kArchitectureNames[architecture]);
}

WString Expand(WString::View value, BuildPlatform platform, Architecture architecture)
{
	return Replace(value, L"$(Architecture)", ToWString(ArchitectureName(platform, architecture)));
}

CString Path(const Project & project, const PathDesc & path, BuildPlatform platform, Architecture architecture)
{
	return EncodeUTF8(Expand(ResolvePath(project.GetRoot(), path), platform, architecture));
}

// Split GNU command-line options, including quoted pkg-config paths. No shell
// evaluation is performed; these tokens are passed directly to IntelliSense.
Array<CString> Arguments(CString::View value)
{
	Array<CString> result;
	CString token;
	char quote = 0;
	bool escaped = false, started = false;
	for (auto c : value)
	{
		if (escaped) { token.Push(c); escaped = false; started = true; }
		else if (c == '\\' && quote != '\'') { escaped = true; started = true; }
		else if (quote) { if (c == quote) quote = 0; else token.Push(c); }
		else if (c == '\'' || c == '"') { quote = c; started = true; }
		else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
		{
			if (started) { result.Push(token); token = {}; started = false; }
		}
		else { token.Push(c); started = true; }
	}
	if (escaped) token.Push('\\');
	if (started) result.Push(token);
	return result;
}

Array<CString> PkgConfigFlags(const TargetConfiguration & config, Map<CString, Array<CString>> & cache)
{
	Array<WString> packages;
	auto append = [&packages](const TargetConfiguration & source)
	{
		for (auto & name : source.GetAppendableStrings(kPkgConfig))
		{
			auto package = ToWString(name);
			if (!Search(packages, package)) packages.Push(package);
		}
	};
	append(config);
	for (auto & dependency : GetLinkDependencies(*config.platform->target, kBuildPlatformLinux))
		if (auto platform = dependency->FindPlatform(kBuildPlatformLinux))
			if (auto source = platform->FindConfiguration(config.GetName())) append(*source);
	if (packages.Empty()) return {};
	Array<CString> names;
	for (auto & package : packages) names.Push(EncodeUTF8(package));
	auto key = JsonArray(names);
	if (auto cached = cache.Search(key)) return *cached;
	auto & result = cache.Acquire(key);
	auto temporary = File::AcquireTempFile(L"reflex-vscode-pkg-");
	if (!temporary.b) return {};
	Array<WString> args = { L"--cflags" };
	args.Append(packages);
	const bool success = RunCommand(L"/usr/bin/pkg-config", args, &*temporary.b);
	temporary.b->SetPosition(0);
	WString line, flags;
	while (File::ReadLine(*temporary.b, line)) flags.Append(Join(line, L" "));
	File::DeleteTempFile(temporary);
	if (!success)
	{
		output.Warn("VS Code: pkg-config flags unavailable; install the target's development packages and regenerate.");
		return {};
	}
	result = Arguments(EncodeUTF8(flags));
	return result;
}

Array<CString> CompilerFlags(const TargetConfiguration & config, Architecture architecture, bool cpp)
{
	Array<CString> flags;
	flags.Append(CopyOwned(GnuCompileOptions(config, config.platform->GetPlatform() == kBuildPlatformLinux, cpp, cpp)));
	flags.Push("-fPIC");
	if (architecture == kArchitecture_x64) flags.Push("-m64");
	if (cpp)
	{
		switch (config.GetEnum<FloatingPoint>(kFloatingPoint, kFloatingPointNames, kFloatingPoint_default))
		{
		case kFloatingPoint_fast: flags.Push("-ffast-math"); break;
		case kFloatingPoint_strict: flags.Push("-frounding-math"); flags.Push("-fsignaling-nans"); break;
		default: break;
		}
		for (auto & option : config.GetStrings(kCompilerOptions))
			flags.Push(EncodeUTF8(Expand(ToWString(option), kBuildPlatformLinux, architecture)));
	}
	return flags;
}

CString LinuxDatabase(const TargetConfiguration & primary, Architecture architecture, Map<CString, Array<CString>> & cache)
{
	Array<CString> entries;
	Map<WString> visited;
	auto append = [&entries, &visited, &cache, architecture](const TargetConfiguration & config)
	{
		auto & project = *config.platform->target->project;
		auto package_flags = PkgConfigFlags(config, cache);
		for (auto & source : FlattenPaths(*config.GetPaths(false, kSources)))
		{
			auto filename = Expand(ResolvePath(project.GetRoot(), source), kBuildPlatformLinux, architecture);
			if (visited.Search(filename)) continue;
			visited.Set(filename);
			auto path = EncodeUTF8(filename);
			bool cpp = !(path.GetSize() >= 2 && Right(path, 2) == ".c");
			Array<CString> arguments = { cpp ? "g++" : "gcc" };
			arguments.Append(CompilerFlags(config, architecture, cpp));
			for (auto & define : config.GetDefinitions()) arguments.Push(Join("-D", EncodeUTF8(Expand(ToWString(define), kBuildPlatformLinux, architecture))));
			for (auto & include : FlattenPaths(*config.GetPaths(true, kIncludeDirectories)))
				arguments.Push(Join("-I", Path(project, include, kBuildPlatformLinux, architecture)));
			arguments.Append(package_flags);
			arguments.Push("-c"); arguments.Push(path);
			entries.Push(Join("{\"directory\":", Json(EncodeUTF8(project.GetRoot())), ",\"file\":", Json(path), ",\"arguments\":", JsonArray(arguments), '}'));
		}
	};
	append(primary);
	for (auto & dependency : GetLinkDependencies(*primary.platform->target, kBuildPlatformLinux))
		if (auto platform = dependency->FindPlatform(kBuildPlatformLinux))
			if (auto config = platform->FindConfiguration(primary.GetName())) append(*config);
	return Join("[\n", Merge(entries, ",\n"), "\n]\n");
}

CString Product(const TargetConfiguration & config, Architecture architecture)
{
	auto & target = *config.platform->target;
	auto & project = *target.project;
	auto platform = config.platform->GetPlatform();
	auto directory = config.GetPath(kOutputDirectory);
	auto output = directory.path ? Path(project, directory, platform, architecture) :
		EncodeUTF8(Join(GetProjectFolder(project, platform), L"output/", ToWString(config.GetName()), L'/', ToWString(ArchitectureName(platform, architecture)), L'/', ToWString(target.GetName())));
	auto name = EncodeUTF8(Expand(ToWString(config.GetString(kOutputName, project.GetName())), platform, architecture));
	auto extension = EncodeUTF8(Expand(ToWString(config.GetString(kOutputExtension)), platform, architecture));
	if (platform == kBuildPlatformWindows && !extension) extension = "exe";
	if (platform == kBuildPlatformMacOS && config.GetProductType() == kOutputType_app)
		return Join(output, '/', name, '.', extension ? extension : CString("app"), "/Contents/MacOS/", name);
	return Join(output, '/', name, extension ? Join('.', extension) : CString {});
}

void Save(const WString & directory, WString::View name, CString::View contents, Map<WString> & outputs)
{
	auto filename = Join(directory, name);
	SaveMetaFile(filename, Data::Pack(contents), kMetaSystemVSCode);
	outputs.Set(filename);
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::GenerateVSCodeProject(Project & project, Array<const TargetPlatform *> & generated_platforms, Map<WString> & outputs)
{
	// This immediate child of the resolved project root is entirely generated.
	const auto root = File::CorrectTrailingStroke(File::ResolveRelativePath(project.GetRoot()));
	const auto directory = Join(root, L".vscode/");
	auto folder_name = WString::View(directory);
	--folder_name.size;
	Require(directory != root && File::SplitFilename(folder_name).a == root, "vscode", "configuration directory must be an immediate child of the project root");
	File::DeletePath(directory);
	File::MakePath(directory);

	Array<CString> configurations, tasks, launches, build_labels;
	Array<CString> recommendations = { "ms-vscode.cpptools" };
	constexpr CString::View generate = "Generate";
	// Use the CLI's host defaults, without rewriting the active task files.
	Array<CString> generate_args = { "generate", "--path", EncodeUTF8(project.GetCfgPath()), "--vscode", "false" };
	tasks.Push(Join("{\"label\":", VSCode::Json(generate), ",\"type\":\"process\",\"command\":", VSCode::Json(EncodeUTF8(System::GetExecutablePath())), ",\"args\":", VSCode::JsonArray(generate_args), ",\"options\":{\"cwd\":\"${workspaceFolder}\"},\"problemMatcher\":[]}"));
	CString default_build;
	Map<CString, Array<CString>> package_flags;
	UInt database_index = 0;
	for (auto & target : project.GetTargets())
	{
		if (target->IsLibrary()) continue;
		for (auto & platform : target->GetTargetPlatforms())
		{
			auto kind = platform->GetPlatform();
			if (!VSCode::Supports(kind)) continue;
			generated_platforms.Push(platform.Adr());
			Array<const TargetConfiguration *> ordered_configs;
			for (auto & config : platform->GetTargetConfigurations())
				if (CaseInsensitive::eq(config->GetName(), kDebug)) ordered_configs.Push(config.Adr());
			for (auto & config : platform->GetTargetConfigurations())
				if (!CaseInsensitive::eq(config->GetName(), kDebug)) ordered_configs.Push(config.Adr());
			for (auto config : ordered_configs)
			{
				auto build = Join("Build ", kBuildPlatforms[kind], ' ', config->GetName());
				if (!Search(build_labels, build))
				{
					build_labels.Push(build);
					bool native = UInt(kind) == UInt(System::kPlatform);
					bool is_default = native && !default_build && CaseInsensitive::eq(config->GetName(), kDebug);
					if (is_default) default_build = build;
					const auto executable = EncodeUTF8(System::GetExecutablePath());
					const auto project_path = EncodeUTF8(project.GetCfgPath());
					Array<CString> generate_command = { executable, "generate", CString(kBuildPlatforms[kind]), "--path", project_path, "--vscode", "false" };
					Array<CString> build_command = { executable, "build", CString(kBuildPlatforms[kind]), CString(config->GetName()), "--path", project_path };
					// One task generates only its platform, then builds if generation succeeds.
					const bool windows = System::kPlatform == System::kPlatformWindows;
					Array<CString> args;
					if (windows)
						args = { "-NoProfile", "-NonInteractive", "-Command", Join(VSCode::ShellCommand(generate_command), "; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; ", VSCode::ShellCommand(build_command), "; exit $LASTEXITCODE") };
					else args = { "-c", Join(VSCode::ShellCommand(generate_command), " && ", VSCode::ShellCommand(build_command)) };
					tasks.Push(Join("{\"label\":", VSCode::Json(build), ",\"type\":\"process\",\"command\":", VSCode::Json(windows ? "powershell.exe" : "/bin/sh"), ",\"args\":", VSCode::JsonArray(args),
						",\"options\":{\"cwd\":\"${workspaceFolder}\"},\"group\":{\"kind\":\"build\",\"isDefault\":", is_default ? "true" : "false",
						"},\"problemMatcher\":", kind == kBuildPlatformWindows ? "\"$msCompile\"" : "\"$gcc\"", '}'));
				}
				// Cross-target build tasks are available immediately; native IntelliSense
				// and debugging are configured against the host toolchain for now.
				if (UInt(kind) != UInt(System::kPlatform)) continue;
				for (auto architecture : config->GetArchitectures())
				{
					Array<CString> includes, defines, flags;
					for (auto & include : FlattenPaths(*config->GetPaths(true, kIncludeDirectories))) includes.Push(VSCode::Path(project, include, kind, architecture));
					for (auto & define : config->GetDefinitions()) defines.Push(EncodeUTF8(VSCode::Expand(ToWString(define), kind, architecture)));
					auto label = Join(kBuildPlatforms[kind], " / ", target->GetName(), " / ", config->GetName(), " / ", kArchitectureNames[architecture]);
					auto standard = config->GetEnum<CppStandard>(kCppStandard, kCppStandardNames, kCppStandard_cxx20);
					auto entry = Join("{\"name\":", VSCode::Json(label), ",\"includePath\":", VSCode::JsonArray(includes), ",\"defines\":", VSCode::JsonArray(defines), ",\"cppStandard\":", standard == kCppStandard_cxx17 ? "\"c++17\"" : "\"c++20\"");
					if (kind == kBuildPlatformLinux)
					{
						auto database = Join(L"compile_commands_", ToWString(database_index++), L".json");
						VSCode::Save(directory, database, VSCode::LinuxDatabase(*config, architecture, package_flags), outputs);
						entry.Append(Join(",\"compilerPath\":\"/usr/bin/g++\",\"compileCommands\":", VSCode::Json(EncodeUTF8(Join(directory, database)))));
						flags = VSCode::CompilerFlags(*config, architecture, true);
						flags.Append(VSCode::PkgConfigFlags(*config, package_flags));
					}
					else if (kind == kBuildPlatformMacOS)
					{
						entry.Append(",\"compilerPath\":\"/usr/bin/clang++\"");
						entry.Append(Join(",\"intelliSenseMode\":", VSCode::Json(architecture == kArchitecture_arm64 ? "macos-clang-arm64" : "macos-clang-x64")));
						flags.Append(CopyOwned(GnuCompileOptions(*config, false, true, true)));
						flags.Push("-arch"); flags.Push(VSCode::ArchitectureName(kind, architecture));
						for (auto & option : config->GetStrings(kCompilerOptions)) flags.Push(EncodeUTF8(VSCode::Expand(ToWString(option), kind, architecture)));
					}
					else
					{
						entry.Append(Join(",\"intelliSenseMode\":", VSCode::Json(architecture == kArchitecture_x86 ? "windows-msvc-x86" : "windows-msvc-x64")));
						flags = config->GetStrings(kCompilerOptions);
						if (standard == kCppStandard_cxx20) flags.Push("/Zc:preprocessor");
					}
					entry.Append(Join(",\"compilerArgs\":", VSCode::JsonArray(flags), '}'));
					configurations.Push(std::move(entry));
					auto type = config->GetProductType();
					if (type != kOutputType_app && type != kOutputType_console) continue;
					auto launch = Join("{\"name\":", VSCode::Json(label), ",\"request\":\"launch\",\"program\":", VSCode::Json(VSCode::Product(*config, architecture)),
						",\"cwd\":\"${workspaceFolder}\",\"args\":[],\"preLaunchTask\":", VSCode::Json(build));
					if (kind == kBuildPlatformWindows) launch.Append(",\"type\":\"cppvsdbg\"");
					else if (kind == kBuildPlatformMacOS)
					{
						launch.Append(",\"type\":\"lldb\"");
						if (!Search(recommendations, "vadimcn.vscode-lldb")) recommendations.Push("vadimcn.vscode-lldb");
					}
					else launch.Append(",\"type\":\"cppdbg\",\"MIMode\":\"gdb\",\"miDebuggerPath\":\"/usr/bin/gdb\",\"setupCommands\":[{\"text\":\"-enable-pretty-printing\",\"ignoreFailures\":true}]");
					launch.Push('}'); launches.Push(std::move(launch));
				}
			}
		}
	}
	if (configurations.Empty()) configurations.Push("{\"name\":\"Default\",\"includePath\":[\"${workspaceFolder}/**\"],\"cppStandard\":\"c++20\"}");
	VSCode::Save(directory, L"c_cpp_properties.json", Join("{\"version\":4,\"configurations\":[\n", Merge(configurations, ",\n"), "\n]}\n"), outputs);
	VSCode::Save(directory, L"tasks.json", Join("{\"version\":\"2.0.0\",\"tasks\":[\n", Merge(tasks, ",\n"), "\n]}\n"), outputs);
	VSCode::Save(directory, L"launch.json", Join("{\"version\":\"0.2.0\",\"configurations\":[\n", Merge(launches, ",\n"), "\n]}\n"), outputs);
	VSCode::Save(directory, L"extensions.json", Join("{\"recommendations\":", VSCode::JsonArray(recommendations), "}\n"), outputs);
	// Examples may also contain independent CMake projects. Opening the folder
	// should use Reflex tasks and IntelliSense without triggering CMake setup.
	VSCode::Save(directory, L"settings.json",
		"{\n"
		"  \"cmake.configureOnOpen\": false,\n"
		"  \"cmake.configureOnEdit\": false,\n"
		"  \"C_Cpp.default.configurationProvider\": \"\",\n"
		"  \"C_Cpp.debugShortcut\": false\n"
		"}\n", outputs);
}
