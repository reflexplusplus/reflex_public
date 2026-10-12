#include "project_gen_emitter.h"
#include "resources.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen::Linux)

Array<CString::View> GccFloatingPointOptions(const TargetConfiguration & config)
{
	REFLEX_STATIC_ASSERT(kFloatingPointCount == 4);

	switch (config.GetEnum<FloatingPoint>(kFloatingPoint, kFloatingPointNames, kFloatingPoint_default))
	{
	case kFloatingPoint_default:
	case kFloatingPoint_precise:
		return {};

	case kFloatingPoint_fast:
		return { kGnuFloatingPointFast };

	case kFloatingPoint_strict:
		return { "-frounding-math", "-fsignaling-nans" };

	default:
		REFLEX_ASSERT(false);
		return {};
	}
}

WString TranslateVariables(WString::View value)
{
	return ProjectGen::TranslateVariables(value,
	{
		{ "Architecture", L"$(ARCHITECTURE)" },
	});
}

CString MakeIdentifier(CString::View value)
{
	CString result;
	for (auto c : value)
	{
		bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
		result.Push(valid ? c : '_');
	}
	return result;
}

CString PathValue(CString::View value)
{
	if (value.size >= 3 && value[1] == ':' && value[2] == '/')
	{
		auto drive = value[0];
		if (drive >= 'A' && drive <= 'Z') drive += 'a' - 'A';
		return Join("/mnt/", drive, Mid(value, 2));
	}
	return value;
}

CString MakeWord(CString::View value)
{
	CString result;
	for (auto c : value)
	{
		if (c == '#' || c == ':') result.Push('\\');
		result.Push(c);
	}
	// Escape spaces after expanding path variables, including PROJECT_ROOT.
	return Join("$(subst $(space),\\$(space),", result, ')');
}

CString Path(const PathDesc & path)
{
	if (path.path.Empty()) return {};
	return PathValue(EncodeUTF8(TranslateVariables(ResolvePath(L"$(PROJECT_ROOT)/", path))));
}

CString OutputDirectory(const Target & target, const TargetConfiguration & config)
{
	auto value = config.GetPath(kOutputDirectory);
	if (!value.path.Empty()) return Path(value);
	return Join("$(MAKEFILE_DIR)/output/$(CONFIGURATION)/$(ARCHITECTURE)/", target.GetName());
}

CString IntermediateDirectory(const Target & target)
{
	return Join("$(MAKEFILE_DIR)/intermediate/$(CONFIGURATION)/$(ARCHITECTURE)/", target.GetName());
}

CString ArchitectureFlags(const TargetConfiguration & config)
{
	CString result;
	for (auto architecture : config.GetArchitectures())
	{
		if (architecture == kArchitecture_x64) result.Append(" -m64");
		else if (architecture != kArchitecture_native) Bootstrap::CLI::ThrowError(Join("unsupported architecture '", kArchitectureNames[architecture], "'"));
	}
	return result;
}

CString PkgConfigPackages(const TargetConfiguration & config)
{
	Array<CString> names;
	auto append = [&names](const TargetConfiguration & source)
	{
		for (auto & package : source.GetAppendableStrings(kPkgConfig))
		{
			if (!Search(names, package)) names.Push(package);
		}
	};
	append(config);
	for (auto & dependency : GetLinkDependencies(*config.platform->target, kBuildPlatformLinux))
	{
		auto platform = dependency->FindPlatform(kBuildPlatformLinux);
		REFLEX_ASSERT(platform);
		if (auto dependency_config = platform->FindConfiguration(config.GetName())) append(*dependency_config);
	}
	CString result;
	for (auto & package : names)
	{
		result.Append(Join(' ', ShellQuote(EncodeUTF8(TranslateVariables(ToWString(package))))));
	}
	return result;
}

bool IsCSource(CString::View path)
{
	return path.size >= 2 && path[path.size - 2] == '.' && path[path.size - 1] == 'c';
}

CString CompileFlags(const TargetConfiguration & config, bool cpp)
{
	CString result = Merge(GnuCompileOptions(config, true, cpp, cpp), " ");
	// Static archives may also be linked into shared plugin modules.
	result.Append(" -fPIC");
	result.Append(ArchitectureFlags(config));
	if (cpp)
	{
		for (auto option : GccFloatingPointOptions(config)) result.Append(Join(' ', option));
		// Configured compiler options may include C++-only flags.
		for (auto & option : config.GetStrings(kCompilerOptions)) result.Append(Join(' ', ShellQuote(EncodeUTF8(TranslateVariables(ToWString(option))))));
	}

	for (auto & define : config.GetDefinitions()) result.Append(Join(" -D", ShellQuote(EncodeUTF8(TranslateVariables(ToWString(define))))));
	for (auto & include : FlattenPaths(*config.GetPaths(true, kIncludeDirectories))) result.Append(Join(" -I", ShellQuote(Path(include))));
	if (auto packages = PkgConfigPackages(config)) result.Append(Join(" $(shell pkg-config --cflags", packages, ")"));
	return result;
}

CString LinkFlags(const TargetConfiguration & config)
{
	CString result = ArchitectureFlags(config);
	if (config.GetProductType() == kOutputType_dynamic_library) result.Append(" -shared");
	if (config.GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static) == kRuntimeLibrary_static)
	{
		result.Append(" -static-libgcc -static-libstdc++");
	}
	auto optimized = config.GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none) != kOptimization_none;
	if (config.GetBool(kDebugInformation, !optimized)) result.Append(" -g");
	if (config.GetBool(kDeadStrip, optimized)) result.Append(" -Wl,--gc-sections");
	return result;
}

CString ProductFilename(const TargetConfiguration & config)
{
	auto output_name = EncodeUTF8(TranslateVariables(ToWString(config.GetString(kOutputName, config.platform->target->project->GetName()))));
	switch (config.GetProductType())
	{
	case kOutputType_console:
	case kOutputType_app:
		if (auto extension = EncodeUTF8(TranslateVariables(ToWString(config.GetString(kOutputExtension))))) return Join(output_name, '.', extension);
		return output_name;
	case kOutputType_static_library: return Join("lib", output_name, ".a");
	case kOutputType_dynamic_library:
	{
		auto extension = EncodeUTF8(TranslateVariables(ToWString(config.GetString(kOutputExtension))));
		if (extension) return Join(output_name, '.', extension);
		return Join("lib", output_name, ".so");
	}
	default: return {};
	}
}

void AddOutput(Map<WString> & outputs, const Target & target, const TargetConfiguration & config)
{
	auto output_directory = config.GetPath(kOutputDirectory);
	if (!output_directory.path) return;
	auto directory = File::CorrectTrailingStroke(ResolvePath(target.project->GetRoot(), output_directory));
	auto filename = ToWString(ProductFilename(config));
	for (auto architecture : config.GetArchitectures())
	{
		auto architecture_name = ToWString(kArchitectureNames[architecture]);
		auto resolved_directory = Replace(directory, L"$(Architecture)", architecture_name);
		auto resolved_filename = Replace(filename, L"$(ARCHITECTURE)", architecture_name);
		outputs.Set(Join(resolved_directory, resolved_filename));
	}
}

CString DependencyTargetPath(const Target & dependency, CString::View configuration)
{
	auto platform = dependency.FindPlatform(kBuildPlatformLinux);
	REFLEX_ASSERT(platform);
	auto config = platform->FindConfiguration(configuration);
	REFLEX_ASSERT(config);
	auto path = config->GetPath(kPath);
	if (!path.path)
	{
		if (!dependency.IsLibrary()) return Join(OutputDirectory(dependency, *config), '/', ProductFilename(*config));
		return Join("-l", dependency.GetName());
	}
	return PathValue(EncodeUTF8(TranslateVariables(ResolvePath(dependency.project->GetRoot(), path))));
}

CString DependencyProjectID(const Project & project)
{
	return Join("dependency_", MakeIdentifier(project.GetName()));
}

CString DependencyTargetID(const Target & target)
{
	return Join("target_", MakeIdentifier(target.GetName()));
}

CString Libraries(const Target & target, const TargetConfiguration & config)
{
	CString result;
	for (auto & dependency : GetLinkDependencies(target, kBuildPlatformLinux)) result.Append(Join(' ', ShellQuote(DependencyTargetPath(*dependency, config.GetName()))));
	if (auto packages = PkgConfigPackages(config)) result.Append(Join(" $(shell pkg-config --libs", packages, ")"));
	result.Append(" -ldl -pthread");
	return result;
}

WString BuildCommand(const BuildActionDesc & action)
{
	WString command = L"cd '$(PROJECT_ROOT)' &&";
	for (auto & argument : action.command)
	{
		command.Push(L' ');
		auto value = TranslateVariables(argument);
		command.Append(ShellQuote(ToWString(PathValue(EncodeUTF8(value)))));
	}
	return command;
}

void WriteActionRecipe(Data::Archive & output, CString::View name, const BuildActionDesc & action)
{
	WriteLine(output, 1, Join("@echo ", ShellQuote(name)));
	WriteLine(output, 1, Join(L"@", BuildCommand(action)));
}

CString WriteBuildActions(Data::Archive & output, const Array<BuildActionDesc> & actions, CString::View id, CString::View phase, CString dependency, CString::View intermediate_directory)
{
	for (UInt i = 0; i < actions.GetSize(); ++i)
	{
		auto & action = actions[i];
		auto & inputs = action.inputs;
		auto & outputs = action.outputs;
		auto name = action.name ? action.name : Join("#", ToCString(i + 1));
		auto action_target = Join(phase, "_", id, "_", ToCString(i));
		CString prerequisites = dependency;
		for (auto & input : inputs) prerequisites.Append(Join(' ', MakeWord(Path(input))));

		if (action.always_run)
		{
			Data::WriteLine(output, Join(".PHONY: ", action_target));
			Data::WriteLine(output, Join(action_target, ": ", prerequisites));
			WriteActionRecipe(output, name, action);
			dependency = action_target;
		}
		else if (outputs)
		{
			CString temp;
			for (auto & path : outputs)
			{
				if (temp) temp.Push(' ');
				temp.Append(MakeWord(Path(path)));
			}
			Data::WriteLine(output, Join(temp, " &: ", prerequisites));
			WriteActionRecipe(output, name, action);
			dependency = MakeWord(Path(outputs[0]));
		}
		else
		{
			auto stamp = MakeWord(Join(intermediate_directory, "/.actions/", action_target, ".stamp"));
			Data::WriteLine(output, Join(stamp, ": ", prerequisites));
			WriteActionRecipe(output, name, action);
			WriteLine(output, 1, "@mkdir -p \"$$(dirname -- \"$@\")\"");
			WriteLine(output, 1, "@touch \"$@\"");
			dependency = stamp;
		}
		Data::WriteLine(output, CString::View {});
	}
	return dependency;
}

bool WriteConfiguration(Data::Archive & output, const Target & target, const TargetConfiguration & config, UInt target_index)
{
	auto output_directory = OutputDirectory(target, config);
	auto target_path = Join(output_directory, '/', ProductFilename(config));
	bool static_lib = false;
	switch (config.GetProductType())
	{
	case kOutputType_console:
	case kOutputType_app:
	case kOutputType_dynamic_library:
		break;

	case kOutputType_static_library:
		static_lib = true;
		break;

	default:
		return false;
	}

	auto architectures = config.GetArchitectures();
	Require(architectures.GetSize() == 1, kArchitectures, "must specify exactly one architecture");
	Data::WriteLine(output, Join("ARCHITECTURE := ", kArchitectureNames[architectures[0]]));
	auto id = MakeIdentifier(Join(target.GetName(), '_', ToCString(target_index)));
	auto intermediate_directory = IntermediateDirectory(target);
	auto flags_name = Join("CXXFLAGS_", id);
	auto c_flags_name = Join("CFLAGS_", id);
	auto pre_build = WriteBuildActions(output, config.GetBuildActions(kBuildPhasePreBuild, System::kPlatformLinux), id, "pre", {}, intermediate_directory);
	CString objects;
	CString object_arguments;
	CString depfiles;
	auto files = GetFiles(config);
	auto sources = FlattenPaths(*files.sources);
	Require(!sources.Empty(), kSources, "undefined");

	bool has_c_sources = false;
	for (UInt i = 0; i < sources.GetSize(); ++i) has_c_sources = has_c_sources || IsCSource(Path(sources[i]));

	Data::WriteLine(output, Join(flags_name, " := ", CompileFlags(config, true)));
	if (has_c_sources) Data::WriteLine(output, Join(c_flags_name, " := ", CompileFlags(config, false)));
	if (!static_lib)
	{
		Data::WriteLine(output, Join("LDFLAGS_", id, " := ", LinkFlags(config)));
		Data::WriteLine(output, Join("LDLIBS_", id, " :=", Libraries(target, config)));
	}

	for (UInt i = 0; i < sources.GetSize(); ++i)
	{
		auto object_path = Join(intermediate_directory, "/", ToCString(i), ".o");
		objects.Append(Join(' ', MakeWord(object_path)));
		object_arguments.Append(Join(' ', ShellQuote(object_path)));
		depfiles.Append(Join(' ', MakeWord(Join(intermediate_directory, "/", ToCString(i), ".d"))));
	}
	Data::WriteLine(output, Join("OBJECTS_", id, " :=", objects));
	Data::WriteLine(output, Join("OBJECT_ARGS_", id, " :=", object_arguments));
	Data::WriteLine(output, Join("DEPFILES +=", depfiles));
	Data::WriteLine(output, CString::View {});

	for (UInt i = 0; i < sources.GetSize(); ++i)
	{
		auto source_path = Path(sources[i]);
		auto object_path = Join(intermediate_directory, "/", ToCString(i), ".o");
		Data::WriteLine(output, Join(MakeWord(object_path), ": ", MakeWord(source_path), pre_build ? Join(" |", pre_build) : CString {}));
		WriteLine(output, 1, "@mkdir -p \"$$(dirname -- \"$@\")\"");
		auto is_c = IsCSource(source_path);
		WriteLine(output, 1, Join(is_c ? "$(CC) $" : "$(CXX) $", "(", is_c ? c_flags_name : flags_name, ") -MMD -MP -c ", ShellQuote(source_path), " -o \"$@\""));
		Data::WriteLine(output, CString::View {});
	}

	CString link_prerequisites;
	Array<CString> order_prerequisites;
	if (!static_lib)
	{
		for (auto & dependency : GetLinkDependencies(target, kBuildPlatformLinux))
		{
			auto path = MakeWord(DependencyTargetPath(*dependency, config.GetName()));
			link_prerequisites.Append(Join(' ', path));
			if (dependency->IsLibrary() || dependency->project == target.project) continue;
			Data::WriteLine(output, Join(path, ": ", DependencyProjectID(*dependency->project)));
		}
	}
	for (auto & dependency : target.GetDependencies(kBuildPlatformLinux))
	{
		if (dependency->IsLibrary()) continue;
		auto prerequisite = dependency->project != target.project ? DependencyProjectID(*dependency->project) : MakeWord(DependencyTargetPath(*dependency, config.GetName()));
		if (!Search(order_prerequisites, prerequisite)) order_prerequisites.Push(std::move(prerequisite));
	}
	Data::WriteLine(output, Join(MakeWord(target_path), ": $(OBJECTS_", id, ")", link_prerequisites, order_prerequisites ? Join(" | ", Merge(order_prerequisites, ' ')) : CString {}));
	WriteLine(output, 1, "@mkdir -p \"$$(dirname -- \"$@\")\"");
	if (static_lib)
	{
		WriteLine(output, 1, Join("$(AR) rcs \"$@\" $(OBJECT_ARGS_", id, ")"));
	}
	else
	{
		WriteLine(output, 1, Join("$(CXX) $(OBJECT_ARGS_", id, ") $(LDFLAGS_", id, ") -o \"$@\" $(LDLIBS_", id, ")"));
	}
	auto final_target = WriteBuildActions(output, config.GetBuildActions(kBuildPhasePostBuild, System::kPlatformLinux), id, "post", MakeWord(target_path), intermediate_directory);
	Data::WriteLine(output, Join("TARGETS += ", final_target));
	Data::WriteLine(output, Join(".PHONY: ", DependencyTargetID(target)));
	Data::WriteLine(output, Join(DependencyTargetID(target), ": ", final_target));
	Data::WriteLine(output, CString::View {});
	return true;
}

void WriteDependencyProjects(Data::Archive & output, Project & project, ArrayView<PlatformTarget> target_platforms)
{
	using DependencyProject = Tuple<ConstTRef<Project>, Array<CString>>;
	Array <DependencyProject> projects;
	for (auto & item : target_platforms)
	{
		auto & target = *item.target;
		if (target.project == project || target.IsLibrary()) continue;
		DependencyProject * dependency_project = SearchValue<KeyCompare>(projects, target.project);
		if (!dependency_project) dependency_project = &projects.Push({ target.project, {} });
		auto id = DependencyTargetID(target);
		if (!Search(dependency_project->b, id)) dependency_project->b.Push(std::move(id));
	}
	Reverse(projects);
	CString previous;
	for (auto & [project,targets] : projects)
	{
		auto id = DependencyProjectID(project);
		auto directory = PathValue(EncodeUTF8(GetProjectFolder(project, kBuildPlatformLinux)));
		Data::WriteLine(output, Join("DEPENDENCIES += ", id));
		Data::WriteLine(output, Join(".PHONY: ", id));
		Data::WriteLine(output, Join(id, previous ? Join(": ", previous) : CString(":")));
		WriteLine(output, 1, Join("@$(MAKE) --no-print-directory -C ", ShellQuote(directory), " CONFIGURATION=", ShellQuote("$(CONFIGURATION)"), ' ', Merge(targets, ' ')));
		Data::WriteLine(output);
		previous = std::move(id);
	}
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::GenerateLinuxProject(Project & project, BuildPlatform, const WString & directory, Array<const TargetPlatform *> & generated_platforms, Map<WString> & outputs)
{
	constexpr CString::View kDebugRelease[] = { "debug", "release" };

	Data::Archive configurations;

	auto generated_count = generated_platforms.GetSize();
	auto target_platforms = CollectTargetPlatforms(project, kBuildPlatformLinux, generated_platforms);
	UInt target_count = 0;
	CString debug_release_configurations[2];
	for (auto & item : target_platforms)
	{
		auto & target = *item.target;
		if (target.project.Adr() == &project && !target.IsLibrary())
		{
			auto platform = item.platform;
			++target_count;
			auto [debug,release] = FindDebugAndReleaseConfigurations(*platform);
			if (debug) debug_release_configurations[0] = debug->GetName();
			if (release) debug_release_configurations[1] = release->GetName();
			for (auto & config : platform->GetTargetConfigurations())
			{
				Data::WriteLine(configurations, Join("ifeq ($(CONFIGURATION),", config->GetName(), ")"));
				Data::WriteLine(configurations, "CONFIGURATION_SUPPORTED := 1");
				if (!Linux::WriteConfiguration(configurations, target, config, target_count - 1))
				{
					generated_platforms.SetSize(generated_count);
					return;
				}
				Data::WriteLine(configurations, "endif");
				Data::WriteLine(configurations);
			}
		}
	}
	if (!target_count) return;
	for (auto & item : target_platforms)
	{
		if (item.target->project.Adr() != &project || item.target->IsLibrary()) continue;
		for (auto & config : item.platform->GetTargetConfigurations()) Linux::AddOutput(outputs, *item.target, *config);
	}
	Linux::WriteDependencyProjects(configurations, project, target_platforms);
	Data::Archive build_helpers;
	REFLEX_LOOP(idx, 2)
	{
		Data::WriteLine(build_helpers, Join(kDebugRelease[idx], ':'));
		if (auto & configuration = debug_release_configurations[idx])
		{
			WriteLine(build_helpers, 1, Join("@$(MAKE) --no-print-directory CONFIGURATION=", configuration, " build"));
		}
		else
		{
			WriteLine(build_helpers, 1, Join("@echo '", kDebugRelease[idx], " configuration is not defined' && false"));
		}
	}
	SaveFile(Join(directory, L"Makefile"), Template(Linux::Makefile,
	{
		{ "MAKEFILE_DIR", ToWString(Linux::PathValue(EncodeUTF8(directory))) },
		{ "PROJECT_ROOT", ToWString(Linux::PathValue(EncodeUTF8(project.GetRoot()))) },
		{ "DEFAULT_CONFIGURATION", ToWString(project.GetDefaultConfiguration()) },
		{ "CONFIGURATIONS", Data::DecodeUTF8(configurations) },
		{ "BUILD_HELPERS", Data::DecodeUTF8(build_helpers) },
	}), kBuildPlatformLinux);

}
