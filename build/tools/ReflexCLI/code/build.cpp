#include "project_gen.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen)

Array<CString> GetConfigurations(const Project & project, BuildPlatform platform, bool products_only = false)
{
	Array<CString> result;
	for (auto & target : project.GetTargets())
	{
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

Array<CString> SelectConfigurations(const Project & project, BuildPlatform platform, CString::View configuration, bool products_only = false)
{
	auto configs = GetConfigurations(project, platform, products_only);
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

void RunXcodeBuild(WString::View workspace, CString::View configuration, WString::View destination, bool quiet, System::FileHandle & std_out)
{
	Array<WString> args;
	if (quiet) args.Push(L"-quiet");
	args.Append({ L"-workspace", File::RemoveTrailingStroke(workspace), L"-scheme", L"Build", L"-configuration", ToWString(configuration), L"-destination", WString(destination), L"build" });
	Require(RunCommand(L"/usr/bin/xcodebuild", args, &std_out), "build failed", workspace);
}

void BuildWithXcode(const Project & project, WString::View directory, BuildPlatform platform, CString::View configuration, CString::View destination, System::FileHandle & std_out)
{
	auto workspace = Join(directory, ToWString(project.GetName()), L".xcworkspace", File::kStroke);
	Require(File::IsDirectory(workspace), "build failed", Join("workspace not found: ", EncodeUTF8(workspace)));
	auto configs = SelectConfigurations(project, platform, configuration);

	for (auto & config : configs)
	{
		if (platform == kBuildPlatformMacOS)
		{
			RunXcodeBuild(workspace, config, L"generic/platform=macOS", true, std_out);
		}
		else
		{
			if (!destination || destination == "all" || destination == "device") RunXcodeBuild(workspace, config, L"generic/platform=iOS", false, std_out);
			if (!destination || destination == "all" || destination == "simulator") RunXcodeBuild(workspace, config, L"generic/platform=iOS Simulator", true, std_out);
		}
	}
}

void BuildWithGradle(const Project & project, WString::View directory, CString::View configuration, System::FileHandle & std_out)
{
	bool application = false;
	for (auto & target : project.GetTargets())
	{
		if (target->IsLibrary()) continue;
		if (auto platform = target->FindPlatform(kBuildPlatformAndroid))
		{
			application = GetOutputType(*platform) == kOutputType_app;
			break;
		}
	}
	auto configs = SelectConfigurations(project, kBuildPlatformAndroid, configuration, true);
	Array<CString::View> variants;
	for (auto & config : configs)
	{
		for (auto & target : project.GetTargets())
		{
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
		if (is_windows)
		{
			Require(RunCommand(L"cmd.exe", { L"/d", L"/c", PlatformPath(wrapper, System::kPlatformWindows), task }, &std_out, true), "build failed", Join(wrapper, L" ", task));
		}
		else
		{
			// Gradle must not read the terminal; the former build helpers redirected stdin to /dev/null.
			Require(RunCommand(L"/bin/sh", { L"-c", L"exec \"$@\" </dev/null", L"reflex-gradle", wrapper, task }, &std_out), "build failed", Join(wrapper, L" ", task));
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

Array<CString> GetWindowsArchitectures(const Project & project, CString::View configuration)
{
	Array<CString> result;
	for (auto & target : project.GetTargets())
	{
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

void BuildWithMsBuild(const Project & project, WString::View directory, CString::View configuration, System::FileHandle & std_out)
{
	auto solution = Join(directory, ToWString(project.GetName()), L".sln");
	Require(File::Exists(solution), "build failed", Join("solution not found: ", EncodeUTF8(solution)));
	auto configs = SelectConfigurations(project, kBuildPlatformWindows, configuration, true);
	auto msbuild = FindMsBuild();
	for (auto & config : configs)
	{
		auto architectures = GetWindowsArchitectures(project, config);
		Require(!architectures.Empty(), "build failed", Join("no Windows architectures for ", config));
		for (auto & architecture : architectures)
		{
			File::WriteLine(std_out, Join("Building ", config, " | ", architecture));
			Array<WString> args = { PlatformPath(solution, System::kPlatformWindows), ToWString(Join("/p:Configuration=", config)), ToWString(Join("/p:Platform=", architecture)), L"/t:Build", L"/m", L"/v:minimal", L"/nologo" };
			Require(RunCommand(msbuild, args, &std_out), "build failed", solution);
		}
	}
}

void BuildWithMake(const Project & project, WString::View directory, CString::View configuration, System::FileHandle & std_out)
{
	auto makefile = Join(directory, L"Makefile");
	Require(File::Exists(makefile), "build failed", Join("Makefile not found: ", EncodeUTF8(makefile)));
	auto configs = SelectConfigurations(project, kBuildPlatformLinux, configuration, true);
	for (auto & config : configs)
	{
		auto setting = Join(L"CONFIGURATION=", ToWString(config));
		if (System::kPlatform == System::kPlatformWindows)
		{
			Require(RunCommand(L"wsl.exe", { L"--cd", PlatformPath(WString(directory), System::kPlatformWindows), L"--exec", L"make", setting, L"build" }, &std_out, true), "build failed", makefile);
		}
		else
		{
			Require(RunCommand(L"/usr/bin/make", { L"-C", WString(directory), setting, L"build" }, &std_out), "build failed", makefile);
		}
	}
}

void BuildWithWeb(const Project & project, WString::View directory, CString::View configuration, System::FileHandle & std_out)
{
	auto script = Join(directory, L"build.py");
	Require(File::Exists(script), "build failed", Join("web build script not found: ", EncodeUTF8(script)));
	auto configs = SelectConfigurations(project, kBuildPlatformWeb, configuration, true);
	const bool is_windows = System::kPlatform == System::kPlatformWindows;
	for (auto & config : configs)
	{
		// NSTask requires an executable path; env resolves Python through PATH.
		if (is_windows)
			Require(RunCommand(L"python", { script, L"--configuration", ToWString(config) }, &std_out, true), "build failed", script);
		else
			Require(RunCommand(L"/usr/bin/env", { L"python3", script, L"--configuration", ToWString(config) }, &std_out, true), "build failed", script);
	}
}

void BuildGeneratedProject(WString::View project_path, CString::View platform_name, BuildPlatform platform, CString::View configuration, CString::View destination, System::FileHandle & std_out)
{
	Array<Reference<Project>> projects;
	auto project = Project::Acquire(projects, project_path);
	auto directory = Join(project->GetRoot(), project->GetGeneratedDirectory(), ToWString(platform_name), File::kStroke);
	Bootstrap::CLI::Print(std_out, Bootstrap::CLI::kColourWhite, project->GetName());
	if (platform == kBuildPlatformIOS || platform == kBuildPlatformMacOS) BuildWithXcode(*project, directory, platform, configuration, destination, std_out);
	else if (platform == kBuildPlatformAndroid) BuildWithGradle(*project, directory, configuration, std_out);
	else if (platform == kBuildPlatformWindows) BuildWithMsBuild(*project, directory, configuration, std_out);
	else if (platform == kBuildPlatformWeb) BuildWithWeb(*project, directory, configuration, std_out);
	else BuildWithMake(*project, directory, configuration, std_out);
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::Build(const WString & path, CString::View platform, CString::View configuration, CString::View destination, System::FileHandle & std_out)
{
	auto platform_index = Search(kBuildPlatforms, platform);
	Require(True(platform_index), "expected platform", "<windows|macos|ios|android|linux|web> [configuration]");
	auto build_platform = BuildPlatform(platform_index.value);
	if (destination)
	{
		Require(build_platform == kBuildPlatformIOS, "unexpected --destination", "only supported for ios builds");
		Require(destination == "all" || destination == "device" || destination == "simulator", "invalid --destination", "expected device, simulator, or all");
	}
	for (auto & source_project : GetSourceProjects(path, build_platform))
	{
		BuildGeneratedProject(source_project, platform, build_platform, configuration, destination, std_out);
	}
	BuildGeneratedProject(path, platform, build_platform, configuration, destination, std_out);
}
