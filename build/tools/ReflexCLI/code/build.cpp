#include "project_gen_emitter.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen)

Array<CString> GetConfigurations(const Project & project, BuildPlatform platform, bool products_only = false, const Target * selected_target = nullptr)
{
	Array<CString> result;
	for (auto & target : project.GetTargets())
	{
		if (selected_target && target.Adr() != selected_target) continue;
		if (products_only && target->IsLibrary()) continue;
		if (auto target_platform = target->FindPlatform(platform))
		{
			for (auto & config : target_platform->GetTargetConfigurations())
			{
				if (!Search<CaseInsensitive>(result, config->GetName())) result.Push(config->GetName());
			}
		}
	}
	return result;
}

Array<CString> SelectConfigurations(const Project & project, BuildPlatform platform, CString::View configuration, bool products_only = false, const Target * selected_target = nullptr)
{
	auto configs = GetConfigurations(project, platform, products_only, selected_target);
	if (configuration)
	{
		auto selected = Search<CaseInsensitive>(configs, configuration);
		Require(True(selected), "build failed", Join("configuration not found: ", configuration));
		Array<CString> selected_configs = { configs[selected.value] };
		configs = std::move(selected_configs);
	}
	Require(!configs.Empty(), "build failed", "nothing to build");
	return configs;
}

void RunXcodeBuild(WString::View workspace, CString::View scheme, CString::View configuration, WString::View destination, bool quiet, System::FileHandle & std_out)
{
	Array<WString> args;
	if (quiet) args.Push(L"-quiet");
	args.Append({ L"-workspace", File::RemoveTrailingStroke(workspace), L"-scheme", ToWString(scheme), L"-configuration", ToWString(configuration), L"-destination", WString(destination), L"build" });
	Require(RunCommand(L"/usr/bin/xcodebuild", args, &std_out), "build failed", workspace);
}

void BuildWithXcode(const Project & project, WString::View directory, BuildPlatform platform, CString::View configuration, CString::View destination, const Target * target, System::FileHandle & std_out)
{
	auto workspace = Join(directory, ToWString(project.GetName()), L".xcworkspace", File::kStroke);
	Require(File::IsDirectory(workspace), "build failed", Join("workspace not found: ", EncodeUTF8(workspace)));
	auto configs = SelectConfigurations(project, platform, configuration, false, target);
	auto scheme = target ? target->GetName() : CString::View("Build");

	for (auto & config : configs)
	{
		if (platform == kBuildPlatformMacOS)
		{
			RunXcodeBuild(workspace, scheme, config, L"generic/platform=macOS", true, std_out);
		}
		else
		{
			if (!destination || destination == "all" || destination == "device") RunXcodeBuild(workspace, scheme, config, L"generic/platform=iOS", false, std_out);
			if (!destination || destination == "all" || destination == "simulator") RunXcodeBuild(workspace, scheme, config, L"generic/platform=iOS Simulator", true, std_out);
		}
	}
}

void BuildWithGradle(const Project & project, WString::View directory, CString::View configuration, const Target * selected_target, System::FileHandle & std_out)
{
	bool application = false;
	for (auto & target : project.GetTargets())
	{
		if (selected_target && target.Adr() != selected_target) continue;
		if (target->IsLibrary()) continue;
		if (auto platform = target->FindPlatform(kBuildPlatformAndroid))
		{
			application = GetOutputType(*platform) == kOutputType_app;
			break;
		}
	}
	auto configs = SelectConfigurations(project, kBuildPlatformAndroid, configuration, true, selected_target);
	Array<CString::View> variants;
	for (auto & config : configs)
	{
		for (auto & target : project.GetTargets())
		{
			if (selected_target && target.Adr() != selected_target) continue;
			if (target->IsLibrary()) continue;
			if (auto platform = target->FindPlatform(kBuildPlatformAndroid))
			{
				auto [debug, release] = FindDebugAndReleaseConfigurations(*platform);
				Require(True(debug) && True(release), "build failed", "Android Debug and Release configurations are required");
				CString::View variant;
				if (CaseInsensitive::eq(config, debug->GetName())) variant = kDebug;
				else if (CaseInsensitive::eq(config, release->GetName())) variant = kRelease;
				if (variant && !Search(variants, variant)) variants.Push(variant);
			}
		}
	}
	Require(!variants.Empty(), "build failed", "no matching Android build variants");
	const bool is_windows = System::kPlatform == System::kPlatformWindows;
	auto wrapper = Join(directory, is_windows ? WString::View(L"gradlew.bat") : WString::View(L"gradlew"));
	Require(File::Exists(wrapper), "build failed", Join("Gradle wrapper not found: ", EncodeUTF8(wrapper)));
	for (auto variant : variants)
	{
		auto task = ToWString(Join(application ? "assemble" : "externalNativeBuild", variant));
		Array<WString> args;
		if (selected_target) args.Push(Join(L"-PreflexTarget=", ToWString(selected_target->GetName())));
		if (is_windows)
		{
			Array<WString> command = { L"/d", L"/c", PlatformPath(wrapper, System::kPlatformWindows), task };
			command.Append(args);
			Require(RunCommand(L"cmd.exe", command, &std_out), "build failed", Join(wrapper, L" ", task));
		}
		else
		{
			// Gradle must not read the terminal; the former build helpers redirected stdin to /dev/null.
			Array<WString> command = { L"-c", L"exec \"$@\" </dev/null", L"reflex-gradle", wrapper, task };
			command.Append(args);
			Require(RunCommand(L"/bin/sh", command, &std_out), "build failed", Join(wrapper, L" ", task));
		}
	}
}

WString FindMsBuild()
{
	auto program_files = File::CorrectStrokes(System::GetEnvironmentVariable(L"ProgramFiles(x86)"));
	Require(True(program_files), "build failed", "ProgramFiles(x86) is not set");
	File::Detail::CorrectTrailingStroke(program_files);
	auto vswhere = Join(program_files, L"Microsoft Visual Studio/Installer/vswhere.exe");
	Require(File::Exists(vswhere), "build failed", Join("vswhere not found: ", EncodeUTF8(vswhere)));
	auto result = File::AcquireTempFile(L"reflex-msbuild-");
	Require(True(result.b), "build failed", "could not create temporary file");
	auto found = RunCommand(vswhere, { L"-latest", L"-products", L"*", L"-requires", L"Microsoft.Component.MSBuild", L"-find", L"MSBuild\\**\\Bin\\amd64\\MSBuild.exe" }, &*result.b);
	result.b->SetPosition(0);
	WString msbuild, line;
	while (File::ReadLine(*result.b, line)) if (Trim(line)) msbuild = File::CorrectStrokes(Trim(line));
	File::DeleteTempFile(result);
	Require(found && File::Exists(msbuild), "build failed", "MSBuild not found by vswhere");
	return msbuild;
}

Array<CString> GetWindowsArchitectures(const Project & project, CString::View configuration, const Target * selected_target)
{
	Array<CString> result;
	for (auto & target : project.GetTargets())
	{
		if (selected_target && target.Adr() != selected_target) continue;
		if (target->IsLibrary()) continue;
		if (auto platform = target->FindPlatform(kBuildPlatformWindows))
		{
			for (auto & config : platform->GetTargetConfigurations())
			{
				if (!CaseInsensitive::eq(config->GetName(), configuration)) continue;
				for (auto architecture : config->GetArchitectures())
				{
					CString::View name = architecture == kArchitecture_x86 ? "Win32" : architecture == kArchitecture_x64 ? "x64" : CString::View {};
					Require(True(name), "build failed", "unsupported Windows architecture");
					if (!Search(result, name)) result.Push(name);
				}
			}
		}
	}
	return result;
}

void BuildWithMsBuild(const Project & project, WString::View directory, CString::View configuration, const Target * target, System::FileHandle & std_out)
{
	auto build_project = target ? Join(GetProjectFolder(*target->project, kBuildPlatformWindows), L"vcxproj/", ToWString(target->GetName()), L".vcxproj") : Join(directory, ToWString(project.GetName()), L".sln");
	Require(File::Exists(build_project), "build failed", Join("build project not found: ", EncodeUTF8(build_project)));
	auto configs = SelectConfigurations(project, kBuildPlatformWindows, configuration, true, target);
	auto msbuild = FindMsBuild();
	for (auto & config : configs)
	{
		auto architectures = GetWindowsArchitectures(project, config, target);
		Require(!architectures.Empty(), "build failed", Join("no Windows architectures for ", config));
		for (auto & architecture : architectures)
		{
			File::WriteLine(std_out, Join("Building ", config, " | ", architecture));
			Array<WString> args = { PlatformPath(build_project, System::kPlatformWindows), ToWString(Join("/p:Configuration=", config)), ToWString(Join("/p:Platform=", architecture)), L"/t:Build", L"/m", L"/v:minimal", L"/nologo" };
			Require(RunCommand(msbuild, args, &std_out), "build failed", build_project);
		}
	}
}

void BuildWithMake(const Project & project, WString::View directory, CString::View configuration, const Target * target, System::FileHandle & std_out)
{
	auto makefile = Join(directory, L"Makefile");
	Require(File::Exists(makefile), "build failed", Join("Makefile not found: ", EncodeUTF8(makefile)));
	auto configs = SelectConfigurations(project, kBuildPlatformLinux, configuration, true, target);
	auto goal = target ? ToWString(GetLinuxBuildTarget(target->GetName())) : WString(L"build");
	for (auto & config : configs)
	{
		auto setting = Join(L"CONFIGURATION=", ToWString(config));
		if (System::kPlatform == System::kPlatformWindows)
		{
			Require(RunCommand(L"wsl.exe", { L"--cd", PlatformPath(WString(directory), System::kPlatformWindows), L"--exec", L"make", setting, goal }, &std_out), "build failed", makefile);
		}
		else
		{
			Require(RunCommand(L"/usr/bin/make", { L"-C", WString(directory), setting, goal }, &std_out), "build failed", makefile);
		}
	}
}

void BuildWithWeb(const Project & project, WString::View directory, CString::View configuration, const Target * target, System::FileHandle & std_out)
{
	auto script = Join(directory, L"build.py");
	Require(File::Exists(script), "build failed", Join("web build script not found: ", EncodeUTF8(script)));
	auto configs = SelectConfigurations(project, kBuildPlatformWeb, configuration, true, target);
	const bool is_windows = System::kPlatform == System::kPlatformWindows;
	for (auto & config : configs)
	{
		Array<WString> args = { script, L"--configuration", ToWString(config) };
		if (target) args.Append({ L"--target", ToWString(target->GetName()) });
		// NSTask requires an executable path; env resolves Python through PATH.
		if (is_windows)
			Require(RunCommand(L"python", args, &std_out), "build failed", script);
		else
		{
			Array<WString> command = { L"python3" };
			command.Append(args);
			Require(RunCommand(L"/usr/bin/env", command, &std_out), "build failed", script);
		}
	}
}

void BuildGeneratedProject(const Project & project, CString::View platform_name, BuildPlatform platform, CString::View configuration, CString::View destination, const Target * target, System::FileHandle & std_out)
{
	auto directory = Join(project.GetRoot(), project.GetGeneratedDirectory(), ToWString(platform_name), File::kStroke);
	Bootstrap::CLI::Print(std_out, Bootstrap::CLI::kColourWhite, project.GetName());
	if (platform == kBuildPlatformIOS || platform == kBuildPlatformMacOS) BuildWithXcode(project, directory, platform, configuration, destination, target, std_out);
	else if (platform == kBuildPlatformAndroid) BuildWithGradle(project, directory, configuration, target, std_out);
	else if (platform == kBuildPlatformWindows) BuildWithMsBuild(project, directory, configuration, target, std_out);
	else if (platform == kBuildPlatformWeb) BuildWithWeb(project, directory, configuration, target, std_out);
	else BuildWithMake(project, directory, configuration, target, std_out);
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::Build(const WString & path, CString::View platform, CString::View configuration, CString::View destination, CString::View target_name, System::FileHandle & std_out)
{
	auto platform_index = Search(kBuildPlatforms, platform);
	Require(True(platform_index), "expected platform", "<windows|macos|ios|android|linux|web> [configuration]");
	auto build_platform = BuildPlatform(platform_index.value);
	if (destination)
	{
		Require(build_platform == kBuildPlatformIOS, "unexpected --destination", "only supported for ios builds");
		Require(destination == "all" || destination == "device" || destination == "simulator", "invalid --destination", "expected device, simulator, or all");
	}
	Array<Reference<Project>> projects;
	auto project = Project::Acquire(projects, path);
	const Target * target = nullptr;
	if (target_name)
	{
		auto selected = project->FindTarget(target_name);
		Require(selected && !selected->IsLibrary(), "build target", Join("compiled target not found: ", target_name));
		Require(selected->FindPlatform(build_platform), "build target", Join(target_name, " does not support ", platform));
		target = selected.Adr();
		SelectConfigurations(*project, build_platform, configuration, true, target);
	}
	for (auto & source_project : GetSourceProjects(path, build_platform, target ? target->GetName() : CString::View {}))
	{
		auto source = Project::Acquire(projects, source_project, project->keymap.Adr());
		BuildGeneratedProject(*source, platform, build_platform, configuration, destination, nullptr, std_out);
	}
	BuildGeneratedProject(*project, platform, build_platform, configuration, destination, target, std_out);
}
