#include "project_gen_emitter.h"
#include "resources.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen::VisualStudio)

constexpr CString::View kFindMsBuild =
"@echo off\r\n"
"for /f \"usebackq delims=\" %%I in (`\"%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\amd64\\MSBuild.exe`) do set \"MSBUILD=%%I\"\r\n"
"if not defined MSBUILD exit /b 1\r\n";

constexpr CString::View kMsBuild =
	"echo Building [CONFIG] ^| [ARCHITECTURE]\r\n"
	"\"%MSBUILD%\" \"%~dp0[PROJECT].sln\" /p:Configuration=\"[CONFIG]\" /p:Platform=[ARCHITECTURE] /t:Build /m /v:minimal /nologo\r\n"
	"if errorlevel 1 exit /b %errorlevel%\r\n";

struct ProjectReference
{
	CString path;
	CString guid;
	bool link = false;
	Reference<Target> target;
};

struct Context
{
	const Project & project;
	const Target & target;
	const TargetPlatform & platform;
	WString solution_directory;
	WString project_directory;
	CString guid;
	CString filename;
	ArrayView<ProjectReference> dependencies;
};

WString TranslateVariables(WString::View value)
{
	return ProjectGen::TranslateVariables(value,
	{
		{ "Architecture", L"$(Platform)" },
	});
}

CString TranslateVariables(CString::View value)
{
	return EncodeUTF8(TranslateVariables(ToWString(value)));
}

WString MakeWindowsRelativePath(WString::View base, WString::View path)
{
	WString normalized_path = path;
	if (base.size > 1 && normalized_path.GetSize() > 1 && base[1] == ':' && normalized_path[1] == ':')
	{
		auto lower = [](WChar value) { return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value; };
		if (lower(base[0]) != lower(normalized_path[0])) return normalized_path;
		normalized_path[0] = base[0];
	}

	auto relative = File::MakeRelativePath(base, normalized_path);
	return Search(relative, L':') ? normalized_path : relative;
}

CString ProjectPath(const Context & context, const PathDesc & path)
{
	auto resolved = TranslateVariables(ResolvePath(context.project.GetRoot(), path));
	
	if (!path.is_absolute) resolved = MakeWindowsRelativePath(context.project_directory, resolved);

	return EncodeUTF8(PlatformPath(resolved, System::kPlatformWindows));
}

CString RootPath(const PathDesc & path)
{
	if (path.path.Empty()) return {};

	return EncodeUTF8(PlatformPath(TranslateVariables(ResolvePath(L"$(PROJECT_ROOT)/", path)), System::kPlatformWindows));
}

CString DirectoryPath(const PathDesc & path)
{
	auto result = RootPath(path);
	if (result && result.GetLast() != '\\') result.Push('\\');
	return result;
}

CString DependencyLibraryPath(const Context & context, const ProjectReference & dependency, CString::View configuration)
{
	auto platform = dependency.target->FindPlatform(kBuildPlatformWindows);
	REFLEX_ASSERT(platform);
	auto config = platform->FindConfiguration(configuration);
	REFLEX_ASSERT(config);
	auto path = config->GetPath(kPath);
	if (!path.path) return Join(dependency.target->GetName(), ".lib");
	auto output = File::ResolveRelativePath(ResolvePath(dependency.target->project->GetRoot(), path));
	auto relative = MakeWindowsRelativePath(context.project_directory, TranslateVariables(output));
	return EncodeUTF8(PlatformPath(relative, System::kPlatformWindows));
}

CString JoinList(ArrayView <CString> values, CString::View inherited)
{
	if (!values) return {};
	Array<CString> translated;
	for (auto & value : values) translated.Push(TranslateVariables(value));
	return Join(Merge(translated, ";"), ";", inherited);
}

CString JoinOptions(ArrayView<CString> values, CString::View inherited)
{
	Array<CString> translated;
	for (auto & value : values) translated.Push(TranslateVariables(value));
	auto result = Merge(translated, " ");
	if (result && inherited) result.Push(' ');
	result.Append(inherited);
	return result;
}

CString Guid(CString::View identity)
{
	auto hex = Data::BytesToHex(Data::Pack(Data::FNV1a64(Data::Pack(identity))));
	return Join("{00000000-0000-0000-", Left(hex, 4), "-", Right(hex, 12), "}");
}

constexpr CString::View kConfigurationTypeOptions[] = { "Application", "Application", "DynamicLibrary", "StaticLibrary" };
constexpr CString::View kSubsystemOptions[] = { "Console", "Windows", "", "" };
constexpr CString::View kCppStandardOptions[] = { "stdcpp17", "stdcpp20" };
constexpr CString::View kWarningLevelOptions[] = { "", "Level1", "Level3", "Level4" };
constexpr CString::View kOptimizationOptions[] = { "Disabled", "MinSpace", "MaxSpeed", "Full" };
constexpr CString::View kFloatingPointOptions[] = { "", "Precise", "Fast", "Strict" };
constexpr CString::View kRuntimeLibraryOptions[][2] = { { "MultiThreaded", "MultiThreadedDebug" }, { "MultiThreadedDLL", "MultiThreadedDebugDLL" }, };

REFLEX_STATIC_ASSERT(GetArraySize(kConfigurationTypeOptions) == kOutputTypeCount);
REFLEX_STATIC_ASSERT(GetArraySize(kSubsystemOptions) == kOutputTypeCount);
REFLEX_STATIC_ASSERT(GetArraySize(kCppStandardOptions) == kCppStandardCount);
REFLEX_STATIC_ASSERT(GetArraySize(kWarningLevelOptions) == kWarningLevelCount);
REFLEX_STATIC_ASSERT(GetArraySize(kOptimizationOptions) == kOptimizationCount);
REFLEX_STATIC_ASSERT(GetArraySize(kFloatingPointOptions) == kFloatingPointCount);
REFLEX_STATIC_ASSERT(GetArraySize(kRuntimeLibraryOptions) == kRuntimeLibraryCount);

CString::View ArchitectureName(Architecture architecture)
{
	constexpr CString::View names[] = { {}, "Win32", "x64", {}, {} };
	
	REFLEX_STATIC_ASSERT(GetArraySize(names) == kArchitectureCount);
	Require(True(names[architecture]) && UInt(architecture) < kArchitectureCount, kArchitectures, "unsupported architecture");

	return names[architecture];
}

CString ProductFilename(const TargetConfiguration & config, Architecture architecture)
{
	auto architecture_name = ToWString(ArchitectureName(architecture));
	auto output_name = Replace(ToWString(config.GetString(kOutputName, config.platform->target->project->GetName())), L"$(Architecture)", architecture_name);
	auto extension = Replace(ToWString(config.GetString(kOutputExtension)), L"$(Architecture)", architecture_name);
	if (!extension)
	{
		constexpr CString::View extensions[] = { "exe", "exe", "dll", "lib" };
		REFLEX_STATIC_ASSERT(GetArraySize(extensions) == kOutputTypeCount);
		extension = ToWString(extensions[config.GetProductType()]);
	}
	return EncodeUTF8(Join(output_name, File::kDot, extension));
}

void AddOutputs(Map<WString> & outputs, const Project & project, ArrayView<PlatformTarget> targets)
{
	for (auto & item : targets)
	{
		if (item.target->project.Adr() != &project || item.target->IsLibrary()) continue;
		for (auto & config : item.platform->GetTargetConfigurations())
		{
			auto output_directory = config->GetPath(kOutputDirectory);
			if (!output_directory.path) continue;
			auto directory = File::CorrectTrailingStroke(ResolvePath(item.target->project->GetRoot(), output_directory));
			for (auto architecture : config->GetArchitectures())
			{
				auto architecture_name = ToWString(ArchitectureName(architecture));
				auto resolved = Replace(directory, L"$(Architecture)", architecture_name);
				auto output_name = Replace(ToWString(config->GetString(kOutputName, item.target->project->GetName())), L"$(Architecture)", architecture_name);
				outputs.Set(Join(resolved, ToWString(ProductFilename(*config, architecture))));
				outputs.Set(Join(resolved, output_name, L".pdb"));
				if (config->GetProductType() == kOutputType_dynamic_library)
				{
					outputs.Set(Join(resolved, output_name, L".lib"));
					outputs.Set(Join(resolved, output_name, L".exp"));
				}
			}
		}
	}
}

void WriteProjectConfiguration(XmlWriter & xml, const TargetConfiguration & config)
{
	for (auto & arch : config.GetArchitectures())
	{
		auto architecture = ArchitectureName(arch);
		XmlScope item(xml, "ProjectConfiguration", { { "Include", Join(config.GetName(), "|", architecture) } });
		xml.Element("Configuration", config.GetName()); xml.Element("Platform", architecture);
	}
}

void WriteProjectConfigurations(XmlWriter & xml, const Context & context)
{
	XmlScope group(xml, "ItemGroup", { { "Label", "ProjectConfigurations" } });

	for (auto & config : context.platform.GetTargetConfigurations()) WriteProjectConfiguration(xml, config);
}

void WriteCompileSettings(XmlWriter & xml, const Context & context, const TargetConfiguration & config)
{
	XmlScope compile(xml, "ClCompile");
	auto cpp_standard = config.GetEnum<CppStandard>(kCppStandard, kCppStandardNames, kCppStandard_cxx20);
	auto warning_level = config.GetEnum<WarningLevel>(kWarningLevel, kWarningLevelNames, kWarningLevel_standard);
	auto optimization = config.GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none);
	auto runtime_library = config.GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static);
	auto floating_point = config.GetEnum<FloatingPoint>(kFloatingPoint, kFloatingPointNames, kFloatingPoint_default);
	auto defines = config.GetDefinitions();
	auto compiler_options = config.GetStrings(kCompilerOptions);
	xml.Element("LanguageStandard", kCppStandardOptions[cpp_standard]);
	xml.WriteBoolElement("UseStandardPreprocessor", cpp_standard == kCppStandard_cxx20);
	xml.WriteBoolElement("StringPooling", true);
	xml.WriteBoolElement("RuntimeTypeInfo", config.GetBool(kRtti, true), {});
	xml.WriteBoolElement("TurnOffAllWarnings", warning_level == kWarningLevel_none);
	xml.Element("WarningLevel", kWarningLevelOptions[warning_level]);
	xml.WriteBoolElement("MultiProcessorCompilation", true);
	xml.Element("Optimization", kOptimizationOptions[optimization]);
	xml.Element("RuntimeLibrary", kRuntimeLibraryOptions[runtime_library][optimization == kOptimization_none]);
	xml.Element("FloatingPointModel", kFloatingPointOptions[floating_point]);
	xml.Element("PreprocessorDefinitions", JoinList(defines, "%(PreprocessorDefinitions)"));
	if (compiler_options) xml.Element("AdditionalOptions", JoinOptions(compiler_options, "%(AdditionalOptions)"));

	Array<CString> includes; 
	
	for (auto & item : FlattenPaths(*config.GetPaths(true, kIncludeDirectories)))
	{
		includes.Push(ProjectPath(context, item));
	}

	xml.Element("AdditionalIncludeDirectories", JoinList(includes, "%(AdditionalIncludeDirectories)"));
}

void WriteLinkSettings(XmlWriter & xml, const Context & context, const TargetConfiguration & config)
{
	auto output_type = config.GetProductType();
	if (output_type != kOutputType_static_library)
	{
		Array <CString> libraries;
		for (auto & dependency : context.dependencies)
		{
			if (dependency.link && !dependency.path) libraries.Push(DependencyLibraryPath(context, dependency, config.GetName()));
		}

		XmlScope link(xml, "Link");
		xml.Element("SubSystem", kSubsystemOptions[output_type]);
		xml.Element("AdditionalDependencies", JoinList(libraries, "%(AdditionalDependencies)"));
		auto optimized = config.GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none) != kOptimization_none;
		auto debug_information = config.GetBool(kDebugInformation, !optimized);
		auto dead_strip = config.GetBool(kDeadStrip, optimized);
		xml.WriteBoolElement("GenerateDebugInformation", debug_information, {});
		xml.WriteBoolElement("OptimizeReferences", dead_strip);
		xml.WriteBoolElement("EnableCOMDATFolding", dead_strip);
	}
}

CString BuildPathList(const Array<PathDesc> & paths)
{
	Array<CString> values;
	for (auto & path : paths) values.Push(RootPath(path));
	return JoinList(values, CString::View {});
}

void WriteBuildActions(XmlWriter & xml, bool post_build, ArrayView <BuildActionDesc> actions, CString::View condition, UInt config_index)
{
	CString previous;
	REFLEX_LOOP(action_index, actions.size)
	{
		auto & action = actions[action_index];
		auto target_name = Join(post_build ? "Post_" : "Pre_", ToCString(config_index), "_", ToCString(action_index));
		Array<XmlAttribute> attributes = { { "Name", target_name }, { "Condition", condition } };

		if (post_build)
		{
			attributes.Push({ "AfterTargets", "PostBuildEvent" });
		}
		else
		{
			attributes.Push({ "BeforeTargets", "PreBuildEvent" });
		}

		if (previous) attributes.Push({ "DependsOnTargets", previous });

		if (!action.always_run && action.inputs && action.outputs)
		{
			attributes.Push({ "Inputs", BuildPathList(action.inputs) });
			attributes.Push({ "Outputs", BuildPathList(action.outputs) });
		}

		XmlScope target(xml, "Target", attributes);

		WString command;
		for (auto & i : action.command)
		{
			command.Append(TranslateVariables(i));
			command.Push(' ');
		}

		command.Pop();

		Array<XmlAttribute> exec =
		{
			{ "Command", EncodeUTF8(command) },
			{ "WorkingDirectory", "$(PROJECT_ROOT)" }
		};
		xml.EmptyElement("Exec", exec);
		previous = std::move(target_name);
	}
}

void WriteFiles(XmlWriter & xml, const Context & context, const TargetConfiguration & config)
{
	XmlScope group(xml, "ItemGroup", { { "Condition", Join("'$(Configuration)'=='", config.GetName(), "'") } });
	
	auto [sources, headers, other] = GetFiles(config);

	for (auto & item : FlattenPaths(sources)) xml.EmptyElement("ClCompile", { { "Include", ProjectPath(context, item) } });
	for (auto & item : FlattenPaths(headers)) xml.EmptyElement("ClInclude", { { "Include", ProjectPath(context, item) } });
	for (auto & item : FlattenPaths(other)) xml.EmptyElement("None", { { "Include", ProjectPath(context, item) } });
}

CString::View WindowsResourceElement(const PathDesc & path)
{
	auto extension = MakeKey32(Lowercase(File::SplitExtension(path.path).b));
	Require(extension == K32("rc") || extension == K32("res"), kWindowsResources, Join("expected an .rc or .res path: ", EncodeUTF8(path.path)));
	return extension == K32("rc") ? CString::View("ResourceCompile") : CString::View("Resource");
}

void WriteWindowsResource(XmlWriter & xml, const Context & context, const PathDesc & path, CString::View filter = {})
{
	XmlScope file(xml, WindowsResourceElement(path), { { "Include", ProjectPath(context, path) } });
	if (!filter && MakeKey32(Lowercase(File::SplitExtension(path.path).b)) == K32("rc"))
	{
		auto [folder,filename] = File::SplitFilename(path.path);
		PathDesc include_directory = { {}, std::move(folder), path.is_absolute };
		xml.Element("AdditionalIncludeDirectories", Join(ProjectPath(context, include_directory), ";%(AdditionalIncludeDirectories)"));
	}
	if (filter) xml.Element("Filter", filter);
}

void WriteFilters(const Context & context, const PathGroup & groups, const PathGroup & resources)
{
	REFLEX_LOCAL(void,WriteGroups)(XmlWriter & xml, const Context & context, const PathGroup & group, CString path, Map<CString,bool> & filters)
	{
		for (auto & child : group)
		{
			auto child_path = path ? Join(path, "\\", child.name) : child.name;
			if (SetFiltered(filters.Acquire(child_path), true))
			{
				XmlScope filter(xml, "Filter", { { "Include", child_path } });
				xml.Element("UniqueIdentifier", Guid(Join(context.guid, ":filter:", child_path)));
			}
			Call(xml, context, child, std::move(child_path), filters);
		}
	};
	REFLEX_END

	REFLEX_LOCAL(void,WriteResourceFiles)(XmlWriter & xml, const Context & context, const PathGroup & group, CString filter)
	{
		for (auto & item : group.paths) WriteWindowsResource(xml, context, item.a, filter);
		for (auto & child : group)
		{
			auto child_filter = Join(filter, "\\", child.name);
			Call(xml, context, child, std::move(child_filter));
		}
	};
	REFLEX_END

	REFLEX_LOCAL(void,WriteFiles)(XmlWriter & xml, const Context & context, const PathGroup & group, CString::View filter)
	{
		for (auto & item : group.paths)
		{
			auto element = item.b == Key32(kSources) ? CString::View("ClCompile") : item.b == Key32(kHeaders) ? CString::View("ClInclude") : CString::View("None");
			REFLEX_ASSERT(item.b == Key32(kSources) || item.b == Key32(kHeaders) || item.b == Key32(kOtherFiles));
			XmlScope file(xml, element, { { "Include", ProjectPath(context, item.a) } });
			if (filter) xml.Element("Filter", filter);
		}
		for (auto & child : group)
		{
			auto child_filter = filter ? Join(filter, "\\", child.name) : child.name;
			Call(xml, context, child, child_filter);
		}
	};
	REFLEX_END

	XmlWriter xml;
	{
		XmlScope root(xml, "Project", { { "ToolsVersion", "4.0" }, { "xmlns", "http://schemas.microsoft.com/developer/msbuild/2003" } });
		Map<CString,bool> filters;

	{
		XmlScope group(xml, "ItemGroup");
		WriteGroups::Call(xml, context, groups, {}, filters);
		if (!resources.paths.Empty() || !resources.Empty())
		{
			auto platform = kBuildPlatforms[kBuildPlatformWindows];
			if (SetFiltered(filters.Acquire(platform), true))
			{
				XmlScope filter(xml, "Filter", { { "Include", platform } });
				xml.Element("UniqueIdentifier", Guid(Join(context.guid, ":filter:", platform)));
			}
			WriteGroups::Call(xml, context, resources, platform, filters);
		}
	}
	{
		XmlScope group(xml, "ItemGroup");
		WriteFiles::Call(xml, context, groups, {});
		if (!resources.paths.Empty() || !resources.Empty()) WriteResourceFiles::Call(xml, context, resources, kBuildPlatforms[kBuildPlatformWindows]);
	}
	}
	SaveFile(Join(context.project_directory, ToWString(context.filename), L".vcxproj.filters"), xml.GetOutput(), kBuildPlatformWindows);
}

void WriteResources(XmlWriter & xml, const Context & context, const PathGroup & resources)
{
	XmlScope group(xml, "ItemGroup");
	for (auto & resource : FlattenPaths(resources)) WriteWindowsResource(xml, context, resource);
}

void WriteProjectReferences(XmlWriter & xml, const Context & context)
{
	if (!context.dependencies) return;
	XmlScope group(xml, "ItemGroup");
	for (auto & dependency : context.dependencies)
	{
		if (!dependency.path) continue;
		XmlScope reference(xml, "ProjectReference", { { "Include", dependency.path } });
		xml.Element("Project", dependency.guid);
		xml.WriteBoolElement("LinkLibraryDependencies", dependency.link, {});
		xml.WriteBoolElement("ReferenceOutputAssembly", false, {});
	}
}

void WriteProject(const Context & context)
{
	auto property_context = NoRetain(context.platform.GetTargetConfigurations()[0]);
	auto windows_sdk = property_context->GetString(kWindowsSdk);
	auto resources = property_context->GetPaths(false, kWindowsResources);
	Require(True(windows_sdk), kWindowsSdk, "is required");

	XmlWriter xml;
	
	{
		XmlScope root(xml, "Project", { { "DefaultTargets", "Build" }, { "xmlns", "http://schemas.microsoft.com/developer/msbuild/2003" } });
	
		WriteProjectConfigurations(xml, context);
		
		{ 
			XmlScope globals(xml, "PropertyGroup", { { "Label", "Globals" } }); 
			xml.Element("VCProjectVersion", "17.0"); xml.Element("ProjectGuid", context.guid); 
			xml.Element("RootNamespace", context.target.GetName());
			xml.Element("WindowsTargetPlatformVersion", windows_sdk);
		}
		
		xml.EmptyElement("Import", { { "Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props" } });
		
		for (auto & config : context.platform.GetTargetConfigurations())
		{
			auto condition = Join("'$(Configuration)'=='", config->GetName(), "'");
			{ 
				XmlScope group(xml, "PropertyGroup", { { "Label", "Configuration" }, { "Condition", condition } }); 
				xml.Element("ConfigurationType", kConfigurationTypeOptions[config->GetProductType()]);
				xml.Element("PlatformToolset", "$(DefaultPlatformToolset)");
				xml.Element("CharacterSet", "Unicode"); 
			}
		}
		
		xml.EmptyElement("Import", { { "Project", "$(VCTargetsPath)\\Microsoft.Cpp.props" } });

		{
			XmlScope imports(xml, "ImportGroup", { { "Label", "PropertySheets" } });
		}
		
		{ 
			XmlScope group(xml, "PropertyGroup"); 
			auto project_root = MakeWindowsRelativePath(context.project_directory, context.project.GetRoot());
			xml.Element("PROJECT_ROOT", EncodeUTF8(PlatformPath(project_root ? project_root : WString(L"."), System::kPlatformWindows)));
		}
		
		auto configurations = context.platform.GetTargetConfigurations();
		REFLEX_LOOP(idx, configurations.size)
		{
			auto & config = configurations[idx];
			auto condition = Join("'$(Configuration)'=='", config->GetName(), "'");

			{
				XmlScope group(xml, "PropertyGroup", { { "Condition", condition } }); 
				auto optimization = config->GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none);
				auto dead_strip = config->GetBool(kDeadStrip, optimization != kOptimization_none);
				auto output_extension = config->GetString(kOutputExtension);
				auto output_directory = config->GetPath(kOutputDirectory);
				auto incremental_link = optimization == kOptimization_none && !dead_strip;
				xml.Element("TargetName", TranslateVariables(config->GetString(kOutputName, context.project.GetName())));
				xml.Element("TargetExt", output_extension ? TranslateVariables(Join(".", output_extension)) : CString {});
				xml.Element("IntDir", "$(SolutionDir)intermediate\\$(Configuration)\\$(Platform)\\$(ProjectName)\\");
				if (config->GetProductType() != kOutputType_static_library) xml.WriteBoolElement("LinkIncremental", incremental_link, {});
				if (config->GetProductType() == kOutputType_app || config->GetProductType() == kOutputType_console) xml.WriteBoolElement("IgnoreImportLibrary", true, {});
				if (!output_directory.path.Empty()) xml.Element("OutDir", DirectoryPath(output_directory));
			}

			{
				XmlScope settings(xml, "ItemDefinitionGroup", { { "Condition", condition } }); 
				WriteCompileSettings(xml, context, config);
				WriteLinkSettings(xml, context, config); 
			}
			
			WriteFiles(xml, context, config);
			WriteBuildActions(xml, false, config->GetBuildActions(kBuildPhasePreBuild, System::kPlatformWindows), condition, idx);
			WriteBuildActions(xml, true, config->GetBuildActions(kBuildPhasePostBuild, System::kPlatformWindows), condition, idx);
		}

		WriteResources(xml, context, *resources);
		WriteProjectReferences(xml, context);

		xml.EmptyElement("Import", { { "Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets" } });
	}
	
	SaveFile(Join(context.project_directory, ToWString(context.filename), L".vcxproj"), xml.GetOutput(), kBuildPlatformWindows);
	auto groups = ConsolidatePathGroups(context.platform);
	WriteFilters(context, *groups, *resources);
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::GenerateVisualStudioProject(Project & project, BuildPlatform, const WString & directory, Array<const TargetPlatform *> & generated_platforms, Map<WString> & outputs)
{
	auto project_directory = Join(directory, L"vcxproj", File::kStroke);

	struct GeneratedTarget
	{
		const PlatformTarget * planned = nullptr;
		CString guid;
		WString project_directory;
		WString project_path;
		CString solution_path;
		OutputType output_type;
		Array<CString> solution_dependencies;
		Array<VisualStudio::ProjectReference> project_references;
	};
	Array<GeneratedTarget> targets;
	Map<Target *,UInt> target_indices;
	auto target_platforms = CollectTargetPlatforms(project, kBuildPlatformWindows, generated_platforms);
	VisualStudio::AddOutputs(outputs, project, target_platforms);

	for (auto & item : target_platforms)
	{
		auto & target = *item.target;
		auto & target_project = *target.project;
		auto platform = item.platform;
		Require(True(platform->GetTargetConfigurations()), "target", Join(target_project.GetName(), "/", target.GetName(), " has no configurations"));
		if (!target.IsLibrary())
		{
			auto sdk = GetConfigurationInvariant(*platform, kWindowsSdk, [](const TargetConfiguration & configuration) { return configuration.GetString(kWindowsSdk); });
			Require(True(sdk), kWindowsSdk, Join(target_project.GetName(), "/", target.GetName(), ": is required"));
			GetConfigurationInvariant(*platform, kWindowsResources,
				[](const TargetConfiguration & configuration) { return FlattenPaths(*configuration.GetPaths(false, kWindowsResources)); },
				[](ArrayView<PathDesc> a, ArrayView<PathDesc> b) { return ComparePaths(a, b); });
		}
		WString target_project_directory;
		WString target_project_path;
		CString solution_path;
		if (!target.IsLibrary())
		{
			target_project_directory = Join(GetProjectFolder(target_project, kBuildPlatformWindows), L"vcxproj", File::kStroke);
			target_project_path = Join(target_project_directory, ToWString(target.GetName()), L".vcxproj");
			solution_path = EncodeUTF8(PlatformPath(VisualStudio::MakeWindowsRelativePath(directory, target_project_path), System::kPlatformWindows));
		}
		target_indices.Set(&target, targets.GetSize());
		targets.Push({ &item, VisualStudio::Guid(Join(target_project.GetName(), ":", target.GetName())), std::move(target_project_directory),
			std::move(target_project_path), std::move(solution_path),
			GetOutputType(*platform) });
	}
	if (target_platforms.Empty()) return;
	File::MakePath(project_directory);

	auto find_target = [&targets, &target_indices](Target & reference) -> GeneratedTarget *
	{
		auto index = target_indices.Search(&reference);
		return index ? &targets[*index] : nullptr;
	};
	for (auto & item : target_platforms)
	{
		auto & reference = *item.target;
		auto target = find_target(reference);
		REFLEX_ASSERT(target);
		auto add_reference = [target](const GeneratedTarget & dependency, bool link)
		{
			for (auto & reference : target->project_references)
			{
				if (reference.target.Adr() != dependency.planned->target.Adr()) continue;
				reference.link |= link;
				return;
			}
			CString path;
			if (!dependency.planned->target->IsLibrary())
			{
				path = EncodeUTF8(PlatformPath(VisualStudio::MakeWindowsRelativePath(target->project_directory, dependency.project_path), System::kPlatformWindows));
			}
			target->project_references.Push({ std::move(path), dependency.guid, link, dependency.planned->target });
		};
		for (auto & dependency_reference : reference.GetDependencies(kBuildPlatformWindows))
		{
			auto dependency = find_target(*dependency_reference);
			REFLEX_ASSERT(dependency);
			if (!dependency->planned->target->IsLibrary()) target->solution_dependencies.Push(dependency->guid);
			add_reference(*dependency, false);
		}
		if (target->output_type != kOutputType_static_library)
		{
			for (auto & dependency_reference : item.link_dependencies)
			{
				auto dependency = find_target(*dependency_reference);
				REFLEX_ASSERT(dependency);
				add_reference(*dependency, true);
			}
		}
	}

	Array <Pair<CString,Array<CString>>> solution_configs;
	for (auto & target : targets)
	{
		if (target.planned->target->project.Adr() != &project || target.planned->target->IsLibrary()) continue;
		VisualStudio::Context context = { *target.planned->target->project, *target.planned->target, *target.planned->platform, directory, target.project_directory,
			target.guid, target.planned->target->GetName(), target.project_references };
		VisualStudio::WriteProject(context);
		for (auto & config : target.planned->platform->GetTargetConfigurations())
		{
			Pair <CString, Array<CString>> * item = SearchValue<KeyCompare>(solution_configs, config->GetName());
			if (!item) item = &solution_configs.Push({ config->GetName() });
			for (auto arch : config->GetArchitectures())
			{
				auto architecture = VisualStudio::ArchitectureName(arch);
				if (!Search(item->b, architecture)) item->b.Push(architecture);
			}
		}
	}
	Sort(targets, [](const GeneratedTarget & a, const GeneratedTarget & b)
	{
		return a.output_type == b.output_type ? a.planned->target->GetName() < b.planned->target->GetName() : a.output_type < b.output_type;
	});

	Data::Archive project_entries;
	for (auto & target : targets)
	{
		if (target.planned->target->IsLibrary()) continue;
		Data::WriteLine(project_entries, Join("Project(\"{BC8A1FFA-BEE3-4634-8014-F334798102B3}\") = \"", target.planned->target->GetName(),
			"\", \"", target.solution_path, "\", \"", target.guid, "\""));
		if (target.solution_dependencies)
		{
			WriteLine(project_entries, 1, "ProjectSection(ProjectDependencies) = postProject");
			for (auto & dependency : target.solution_dependencies) WriteLine(project_entries, 2, Join(dependency, " = ", dependency));
			WriteLine(project_entries, 1, "EndProjectSection");
		}
		Data::WriteLine(project_entries, "EndProject");
	}

	Data::Archive configs;
	for (auto & [config, architectures] : solution_configs)
	{
		for (auto & architecture : architectures) WriteLine(configs, 2, Join(config, "|", architecture, " = ", config, "|", architecture));
	}

	Data::Archive target_configs;
	for (auto & target : targets)
	{
		if (target.planned->target->IsLibrary()) continue;
		for (auto & config : target.planned->platform->GetTargetConfigurations())
		{
			for (auto arch : config->GetArchitectures())
			{
				auto architecture = VisualStudio::ArchitectureName(arch);
				auto name = config->GetName();
				WriteLine(target_configs, 2, Join(target.guid, ".", name, "|", architecture, ".ActiveCfg = ", name, "|", architecture));
				WriteLine(target_configs, 2, Join(target.guid, ".", name, "|", architecture, ".Build.0 = ", name, "|", architecture));
			}
		}
	}

	auto solution = Template(VisualStudio::solution_sln,
	{
		{ "PROJECT_ENTRIES", Data::DecodeUTF8(project_entries) },
		{ "CONFIGS", Data::DecodeUTF8(configs) },
		{ "TARGETS", Data::DecodeUTF8(target_configs) },
	});

	SaveFile(Join(directory, ToWString(project.GetName()), L".sln"), solution, kBuildPlatformWindows);

	Require(!solution_configs.Empty(), "project", "has no configurations");

	for (auto & [config,architectures] : solution_configs)
	{
		Data::Archive script = Data::Pack(VisualStudio::kFindMsBuild);
		for (auto & architecture : architectures)
		{
			script.Append(Template(Data::Pack(VisualStudio::kMsBuild),
			{
				{ "PROJECT", ToWString(project.GetName()) },
				{ "CONFIG", ToWString(config) },
				{ "ARCHITECTURE", ToWString(architecture) },
			}));
		}
		SaveFile(File::SetExtension(Join(directory, L"Build ", ToWString(config)), kBat), script, kBuildPlatformWindows);
	}
	XmlWriter defaults;
	{
		XmlScope root(defaults, "Project");
		XmlScope properties(defaults, "PropertyGroup");
		defaults.Element("Configuration", project.GetDefaultConfiguration());
		defaults.Element("Platform", solution_configs.GetFirst().b.GetFirst());
	}
	SaveFile(Join(directory, L"Directory.Solution.props"), defaults.GetOutput(), kBuildPlatformWindows);
}
