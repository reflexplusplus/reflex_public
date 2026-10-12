#include "project_gen_emitter.h"
#include "resources.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen::Web)

void SaveCommandScript(const WString & path, WString::View text, BuildPlatform platform)
{
	SaveText(path, text, platform);
	SetExecutable(path);
}

CString JsonString(CString::View value)
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

CString JsonStringArray(ArrayView<CString> values)
{
	Array<CString> quoted;
	for (auto & value : values) quoted.Push(JsonString(value));
	return Join('[', Merge(quoted, ','), ']');
}

CString BatchArgument(CString::View value)
{
	// These values also name generated files: quotes/control characters are
	// not supported. Percent expansion is a separate batch-file boundary;
	// delayed expansion is disabled in the generated scripts for literal !.
	CString result = "\"";
	UInt trailing_strokes = 0;
	for (auto c : value)
	{
		Require(UInt8(c) >= 32 && c != '"', "web configuration", "must not contain quotes or control characters");
		if (c == '%') result.Push('%');
		result.Push(c);
		trailing_strokes = c == '\\' ? trailing_strokes + 1 : 0;
	}
	// Windows argv parsing consumes backslashes before the closing quote.
	REFLEX_LOOP(index, trailing_strokes) result.Push('\\');
	result.Push('"');
	return result;
}

CString Path(WString::View root, const PathDesc & path)
{
	auto value = Replace(ResolvePath(root, path), L"$(Architecture)", L"wasm32");
	Require(!Search(value, L"$("), "web path", "contains an unresolved build-time variable");
	return EncodeUTF8(value);
}

CString Product(const Target & target, const TargetConfiguration & config)
{
	auto directory = config.GetPath(kOutputDirectory);
	auto folder = directory.path ? Path(target.project->GetRoot(), directory) :
		EncodeUTF8(Join(GetProjectFolder(*target.project, kBuildPlatformWeb), L"output/", ToWString(config.GetName()), File::kStroke, ToWString(target.GetName()), File::kStroke));
	auto name = config.GetString(kOutputName, target.GetName());
	Require(!Search(name, '/') && !Search(name, '\\') && name != "." && name != "..", kOutputName, "must be a filename");
	return Join(EncodeUTF8(File::CorrectTrailingStroke(ToWString(folder))), config.GetProductType() == kOutputType_static_library ? Join("lib", name, ".a") : Join(name, ".wasm"));
}

CString Dependency(const Target & target, CString::View configuration)
{
	auto platform = target.FindPlatform(kBuildPlatformWeb);
	Require(platform != nullptr, "web dependency", Join(target.GetName(), " has no web platform"));
	auto config = platform->FindConfiguration(configuration);
	Require(config != nullptr, "web dependency", Join(target.GetName(), " has no matching configuration"));
	if (!target.IsLibrary()) return Product(target, *config);
	auto path = config->GetPath(kPath);
	return path.path ? Path(target.project->GetRoot(), path) : Join("-l", target.GetName());
}

CString Actions(const Target & target, const TargetConfiguration & config, BuildPhase phase)
{
	Array<CString> actions;
	for (auto & action : config.GetBuildActions(phase, System::kPlatform))
	{
		Array<CString> command;
		for (auto & argument : action.command) command.Push(EncodeUTF8(argument));
		actions.Push(Join("{\"cwd\":", JsonString(EncodeUTF8(target.project->GetRoot())), ",\"command\":", JsonStringArray(command), '}'));
	}
	return Join('[', Merge(actions, ','), ']');
}

CString Configuration(const Target & target, const TargetConfiguration & config, WString::View launcher)
{
	auto type = config.GetProductType();
	Require(type == kOutputType_app || type == kOutputType_static_library, "web target", "currently supports app and static_library targets");
	auto architectures = config.GetArchitectures();
	Require(architectures.GetSize() == 1 && architectures[0] == kArchitecture_native, kArchitectures, "web uses native (wasm32)");
	Array<CString> sources, flags, libraries, dependencies;
	for (auto & path : FlattenPaths(*GetFiles(config).sources)) sources.Push(Path(target.project->GetRoot(), path));
	Require(True(sources), kSources, "undefined");
	flags.Append(CopyOwned(GnuCompileOptions(config, false, true, true)));
	if (config.GetEnum<FloatingPoint>(kFloatingPoint, kFloatingPointNames, kFloatingPoint_default) == kFloatingPoint_strict)
	{
		// WebAssembly does not support dynamic rounding modes or floating-point exceptions.
		// Preserve the strict mode's supported no-contraction behavior without requesting them.
		flags.Push("-ffp-model=precise");
		flags.Push("-ffp-contract=off");
	}
	else flags.Append(CopyOwned(ClangFloatingPointOptions(config)));
	flags.Append(config.GetStrings(kCompilerOptions));
	for (auto & define : config.GetDefinitions()) flags.Push(Join("-D", define));
	for (auto & path : FlattenPaths(*config.GetPaths(true, kIncludeDirectories))) flags.Push(Join("-I", Path(target.project->GetRoot(), path)));
	for (auto & dependency : target.GetDependencies(kBuildPlatformWeb))
		if (!dependency->IsLibrary()) dependencies.Push(Dependency(*dependency, config.GetName()));
	if (type != kOutputType_static_library)
		for (auto & dependency : GetLinkDependencies(target, kBuildPlatformWeb)) libraries.Push(Dependency(*dependency, config.GetName()));
	return Join("{\"configuration\":", JsonString(config.GetName()), ",\"name\":", JsonString(target.GetName()),
		",\"cwd\":", JsonString(EncodeUTF8(target.project->GetRoot())), ",\"static\":", type == kOutputType_static_library ? "true" : "false",
		",\"product\":", JsonString(Product(target, config)), ",\"launcher\":", JsonString(EncodeUTF8(launcher)), ",\"sources\":", JsonStringArray(sources), ",\"flags\":", JsonStringArray(flags),
		",\"libraries\":", JsonStringArray(libraries), ",\"dependencies\":", JsonStringArray(dependencies), ",\"pre\":", Actions(target, config, kBuildPhasePreBuild),
		",\"post\":", Actions(target, config, kBuildPhasePostBuild), '}');
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::GenerateWebProject(Project & project, BuildPlatform, const WString & directory, Array<const TargetPlatform *> & generated_platforms, Map<WString> & outputs)
{
	auto targets = CollectTargetPlatforms(project, kBuildPlatformWeb, generated_platforms);
	Array<CString> configurations, names, projects;
	CString reflex_path;
	CString run_configurations[2];
	const auto generate_timestamp = ToWString(System::GetTime());
	for (auto & item : targets)
	{
		if (item.target->IsLibrary()) continue;
		if (item.target->project.Adr() != &project)
		{
			auto path = EncodeUTF8(Join(GetProjectFolder(*item.target->project, kBuildPlatformWeb), L"build.py"));
			if (!Search(projects, path)) projects.Push(std::move(path));
			continue;
		}
		if (GetOutputType(*item.platform) == kOutputType_app)
		{
			auto [debug, release] = FindDebugAndReleaseConfigurations(*item.platform);
			if (debug && !run_configurations[0]) run_configurations[0] = debug->GetName();
			if (release && !run_configurations[1]) run_configurations[1] = release->GetName();
		}
		for (auto & config : item.platform->GetTargetConfigurations())
		{
			WString launcher;
			auto product = ToWString(Web::Product(*item.target, config));
			if (config->GetProductType() == kOutputType_app)
			{
				auto launcher_folder = File::CorrectTrailingStroke(ResolveAbsolutePathCase(Join(directory, L"launchers/")));
				launcher = Join(launcher_folder, ToWString(config->GetName()), File::kStroke, ToWString(item.target->GetName()), L"/index.html");
				auto entry_page = config->GetPath(kWebEntryPage);
				auto page = File::Extract(Web::index_html);
				auto [product_path, product_filename] = File::SplitFilename(product);
				if (entry_page.path)
				{
					auto source = ToWString(Web::Path(project.GetRoot(), entry_page));
					Require(File::Exists(source) && !File::IsDirectory(source), kWebEntryPage, Join("launcher file not found: ", EncodeUTF8(source)));
					// Generated launcher artifacts are disposable; never use them as source pages.
					auto source_abs = ResolveAbsolutePathCase(source);
					Require(!CaseInsensitive::eq(Left<true>(source_abs, launcher_folder.GetSize()), launcher_folder), kWebEntryPage, "source page must be outside the generated launchers folder");
					auto packaged_page = Join(product_path, L"index.html");
					Require(!CaseInsensitive::eq(source_abs, ResolveAbsolutePathCase(packaged_page)), kWebEntryPage, "source page must differ from the packaged index.html");
					page = File::Open(source);
					Require(True(page), kWebEntryPage, Join("could not read launcher file: ", EncodeUTF8(source)));
				}
				auto module = Join("./", Data::EncodeUrlSegment(ToCString(product_filename)));
				SaveFile(launcher, Template(page, {
					{ "TITLE", ToWString(EscapeXml(project.GetName())) },
					{ "MODULE", ToWString(module) },
					{ "GENERATE_TIMESTAMP", generate_timestamp }
				}), kBuildPlatformWeb);
			}
			configurations.Push(Web::Configuration(*item.target, config, launcher));
			if (!Search(names, config->GetName())) names.Push(CString(config->GetName()));
			if (!reflex_path) reflex_path = config->GetVariable("REFLEX_PATH");
			outputs.Set(product);
			if (config->GetProductType() == kOutputType_app)
			{
				auto folder = File::SplitFilename(product).a;
				outputs.Set(Join(folder, L"index.html"));
				outputs.Set(Join(folder, L"runtime/"));
			}
		}
	}
	if (configurations.Empty()) return;
	Reverse(projects);
	Require(True(reflex_path), "web toolchain", "REFLEX_PATH is required");
	reflex_path = EncodeUTF8(File::ResolveIncludePath(project.GetRoot(), ToWString(reflex_path)));
	SaveText(Join(directory, L"build-config.json"), Join("{\"reflex\":", Web::JsonString(reflex_path),
		",\"default_configuration\":", Web::JsonString(project.GetDefaultConfiguration()),
		",\"projects\":", Web::JsonStringArray(projects), ",\"targets\":[", Merge(configurations, ','), "]}\n"), kBuildPlatformWeb);
	SaveFile(Join(directory, L"build.py"), File::Extract(Web::build_py), kBuildPlatformWeb);
	SaveFile(Join(directory, L"serve_local.py"), File::Extract(Web::serve_local_py), kBuildPlatformWeb);
	constexpr CString::View kRunNames[] = { "run_local_debug", "run_local_release" };
	REFLEX_LOOP(index, 2)
	{
		auto & configuration = run_configurations[index];
		if (!configuration) continue;
		auto path = Join(directory, ToWString(kRunNames[index]));
		SaveText(Join(path, L".bat"), Join("@echo off\r\nsetlocal DisableDelayedExpansion\r\npython \"%~dp0serve_local.py\" --build --open --configuration ", Web::BatchArgument(configuration), " %*\r\nexit /b %errorlevel%\r\n"), kBuildPlatformWeb);
		Web::SaveCommandScript(Join(path, L".command"), ToWString(Join("#!/bin/sh\nexec python3 \"$(dirname \"$0\")/serve_local.py\" --build --open --configuration ", ShellQuote(configuration), " \"$@\"\n")), kBuildPlatformWeb);
	}
	for (auto & name : names)
	{
		SaveText(Join(directory, MakeBuildScriptName(name), L".bat"), Join("@echo off\r\nsetlocal DisableDelayedExpansion\r\npython \"%~dp0build.py\" --configuration ", Web::BatchArgument(name), "\r\nexit /b %errorlevel%\r\n"), kBuildPlatformWeb);
		Web::SaveCommandScript(Join(directory, MakeBuildScriptName(name), L".command"), ToWString(Join("#!/bin/sh\nexec python3 \"$(dirname \"$0\")/build.py\" --configuration ", ShellQuote(name), "\n")), kBuildPlatformWeb);
	}
}
