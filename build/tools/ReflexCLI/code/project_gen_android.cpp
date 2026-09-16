#include "project_gen_emitter.h"
#include "resources.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen::Android)

constexpr CString::View kSigning =
	"def reflex[CONFIGURATION_ID]SigningPropertiesFile = file([SIGNING_PROPERTIES])\n"
	"if (reflex[CONFIGURATION_ID]SigningPropertiesFile.exists()) {\n"
	"\tdef reflex[CONFIGURATION_ID]SigningProperties = new Properties()\n"
	"\treflex[CONFIGURATION_ID]SigningPropertiesFile.withInputStream { reflex[CONFIGURATION_ID]SigningProperties.load(it) }\n"
	"\n"
	"\tandroid {\n"
	"\t\tsigningConfigs {\n"
	"\t\t\t[CONFIGURATION] {\n"
	"\t\t\t\tdef reflexStoreFile = new File(reflex[CONFIGURATION_ID]SigningProperties['storeFile'])\n"
	"\t\t\t\tstoreFile = reflexStoreFile.isAbsolute() ? reflexStoreFile : new File(reflex[CONFIGURATION_ID]SigningPropertiesFile.parentFile, reflexStoreFile.path)\n"
	"\t\t\t\tstorePassword = reflex[CONFIGURATION_ID]SigningProperties['storePassword']\n"
	"\t\t\t\tkeyAlias = reflex[CONFIGURATION_ID]SigningProperties['keyAlias']\n"
	"\t\t\t\tkeyPassword = reflex[CONFIGURATION_ID]SigningProperties['keyPassword']\n"
	"\t\t\t}\n"
	"\t\t}\n"
	"\n"
	"\t\tbuildTypes {\n"
	"\t\t\t[CONFIGURATION] {\n"
	"\t\t\t\tsigningConfig = signingConfigs.[CONFIGURATION]\n"
	"\t\t\t}\n"
	"\t\t}\n"
	"\t}\n"
	"} else {\n"
	"\tlogger.warn(\"signing_properties file not found: ${reflex[CONFIGURATION_ID]SigningPropertiesFile}\")\n"
	"\tandroidComponents {\n"
	"\t\tbeforeVariants(selector().withBuildType(\"[CONFIGURATION]\")) { variantBuilder ->\n"
	"\t\t\tvariantBuilder.enable = false\n"
	"\t\t}\n"
	"\t}\n"
	"}\n";

CString Symbol(CString::View value)
{
	CString result;
	for (auto c : value)
	{
		bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
		result.Push(valid ? c : '_');
	}
	if (result.Empty() || (result[0] >= '0' && result[0] <= '9')) result = Join("App_", result);
	return result;
}

WString RuntimeLibraryName(RuntimeLibrary runtime_library)
{
	return ToWString(runtime_library == kRuntimeLibrary_static ? ToView("c++_static") : ToView("c++_shared"));
}

struct BuildTarget
{
	const Target * target = nullptr;
	const TargetPlatform * platform = nullptr;
	const TargetConfiguration * debug = nullptr;
	const TargetConfiguration * release = nullptr;
	CString library;
	CString output_library;
	CString cmake_identifier;
	CString cmake_package;
	CString cmake_module;
};

CString CMakeIdentifier(const Target & target)
{
	CString configured;
	bool initialized = false;
	for (auto & platform : target.GetTargetPlatforms())
	{
		for (auto & config : platform->GetTargetConfigurations())
		{
			auto value = config->GetString(kCMakeIdentifier);
			if (!initialized)
			{
				configured = std::move(value);
				initialized = true;
			}
			else Require(value == configured, kCMakeIdentifier, Join(target.GetName(), ": cannot vary by platform or configuration"));
		}
	}
	if (configured) return configured;
	return Search(target.GetName(), "::") ? CString(target.GetName()) : Symbol(target.GetName());
}

CString SourceTargetIdentifier(const Target & target)
{
	return Symbol(Replace(CMakeIdentifier(target), "::", "_"));
}

void SplitCMakeIdentifier(CString::View identifier, CString & package, CString & module)
{
	auto separator = Search(identifier, "::");
	Require(separator && separator.value && separator.value + 2 < identifier.size && !Search(Mid(identifier, separator.value + 2), "::"), kCMakeIdentifier,
		Join("expected namespace::target: ", identifier));
	package = Left(identifier, separator.value);
	module = Mid(identifier, separator.value + 2);
	Require(Symbol(package) == package && Symbol(module) == module, kCMakeIdentifier, Join("must contain valid CMake identifiers: ", identifier));
}

WString GradleQuote(WString::View value)
{
	return Join(L'"', EscapeQuotedString(value), L'"');
}

WString CMakeQuote(WString::View value)
{
	return Join(L'"', EscapeQuotedString(value, L';'), L'"');
}

CString GradleQuote(CString::View value)
{
	return EncodeUTF8(GradleQuote(ToWString(value)));
}

WString Variables(WString::View input)
{
	return ProjectGen::TranslateVariables(input,
	{
		{ "Architecture", L"${ANDROID_ABI}" },
	});
}

CString AssembledValue(CString::View input)
{
	return EncodeUTF8(Replace(ToWString(input), L"$(Architecture)", L"universal"));
}

WString Path(const PathDesc & path)
{
	return Variables(ResolvePath(L"${PROJECT_DIR}/", path));
}

WString::View GradleValue(WString::View input)
{
	Require(!Search(input, L"$(Architecture)"), kArchitectures, "cannot be used by a Gradle build action because a configuration may contain multiple ABIs");
	return input;
}

WString GradlePath(const PathDesc & path)
{
	return GradleValue(ResolvePath(L"$projectRoot/", path));
}

void WriteGradlePaths(Data::Archive & output, CString::View member, const Array<PathDesc> & paths)
{
	if (paths)
	{
		WString line = Join(ToWString(member), L'(');
		for (auto & i : paths)
		{
			line.Append(GradleQuote(GradlePath(i)));
			line.Append(L", ");
		}
		line.Shrink(2);
		line.Push(L')');
		WriteLine(output, 1, line);
	}
}

void WriteBuildActions(Data::Archive & output, CString::View name, const TargetConfiguration & config)
{
	CString previous;
	auto actions = config.GetBuildActions(kBuildPhasePreBuild, System::kPlatformAndroid);
	for (UInt index = 0; index < actions.GetSize(); ++index)
	{
		auto & action = actions[index];
		auto task = Join("preBuild", name, ToCString(index));
		Data::WriteLine(output, Join("def ", task, " = tasks.register('", task, "', Exec) {"));
		if (previous) WriteLine(output, 1, Join("dependsOn ", previous));
		WriteLine(output, 1, "workingDir projectRoot");
		WString command = L"commandLine ";
		for (UInt i = 0; i < action.command.GetSize(); ++i)
		{
			if (i) command.Append(L", ");
			command.Append(GradleQuote(GradleValue(action.command[i])));
		}
		WriteLine(output, 1, command);
		WriteGradlePaths(output, "inputs.files", action.inputs);
		WriteGradlePaths(output, "outputs.files", action.outputs);
		if (action.always_run) WriteLine(output, 1, "outputs.upToDateWhen { false }");
		Data::WriteLine(output, "}");
		Data::WriteLine(output);
		previous = task;
	}
	if (previous)
	{
		Data::WriteLine(output, Join("tasks.matching { it.name == 'pre", name, "Build' }.configureEach { dependsOn ", previous, " }"));
		Data::WriteLine(output);
	}
}

void WriteArtifactCopy(Data::Archive & output, CString::View name, const TargetConfiguration & config)
{
	auto output_directory = config.GetPath(kOutputDirectory);
	if (output_directory.path.Empty()) return;

	auto task = Join("copy", name, "Artifact");
	auto build_type = Lowercase(name);
	auto build_task = Join("assemble", name);
	auto source = Join("outputs/apk/", build_type);
	auto filename = Join(AssembledValue(config.GetString(kOutputName, config.platform->target->project->GetName())), ".apk");
	Data::WriteLine(output, Join("def ", task, " = tasks.register('", task, "', Copy) {"));
	WriteLine(output, 1, Join("dependsOn '", build_task, "'"));
	WriteLine(output, 1, Join("onlyIf { tasks.named('", build_task, "').get().state.failure == null }"));
	WriteLine(output, 1, Join("from layout.buildDirectory.dir('", source, "')"));
	WriteLine(output, 1, "include '*.apk'");
	WriteLine(output, 1, Join(L"into file(", GradleQuote(Replace(ResolvePath(L"$projectRoot/", output_directory), L"$(Architecture)", L"universal")), L')'));
	WriteLine(output, 1, Join("rename { _ -> ", GradleQuote(filename), " }"));
	Data::WriteLine(output, "}");
	Data::WriteLine(output, Join("tasks.matching { it.name == 'assemble", name, "' }.configureEach { finalizedBy ", task, " }"));
	Data::WriteLine(output);
}

void WriteDependencies(Data::Archive & output, bool release, const TargetConfiguration & config)
{
	const auto scope = release ? WString::View(L"releaseImplementation") : WString::View(L"debugImplementation");
	Array<WString> file_dependencies;
	const auto write_file_dependency = [&output, scope, &file_dependencies](WString value)
	{
		if (Search(file_dependencies, value)) return;
		file_dependencies.Push(value);
		WriteLine(output, 1, Join(scope, L"(files(", GradleQuote(value), L"))"));
	};

	auto dependencies = config.GetStrings(kAndroidDependencies);

	REFLEX_LOOP(index, dependencies.GetSize())
	{
		auto & dependency = dependencies[index];
		Require(True(dependency), kAndroidDependencies, Join("value must not be empty: ", ToCString(index)));

		auto wdependency = ToWString(dependency);

		if (Search(wdependency, File::kStroke) || System::IsAbsolutePath(wdependency))
		{
			write_file_dependency(GradlePath(DecodePath(dependency)));
		}
		else
		{
			UInt separators = 0;
			bool empty_part = true;
			bool invalid = false;
			for (auto c : dependency)
			{
				if (c == ':')
				{
					invalid = invalid || empty_part;
					++separators;
					empty_part = true;
				}
				else empty_part = false;
			}
			Require(separators == 2 && !empty_part && !invalid, kAndroidDependencies, Join("expected a Maven group:artifact:version coordinate or file path: ", ToCString(index)));
			WriteLine(output, 1, Join(scope, L' ', GradleQuote(GradleValue(wdependency))));
		}
	}
}

UInt32 SdkVersion(const TargetConfiguration & config, CString::View property, UInt32 fallback)
{
	return UInt32(Max(config.GetNumber(property, Float32(fallback)), 0.0f));
}

REFLEX_INLINE WString Text(Data::Archive::View value)
{
	return Data::DecodeUTF8(value);
}

void OverlaySourceSet(const WString & source, const WString & destination)
{
	if (source.Empty()) return;
	auto [folders, files] = File::List(source, true);
	for (auto & folder : folders) OverlaySourceSet(Join(source, folder.key), Join(destination, folder.key));
	for (auto & file : files)
	{
		auto input = Join(source, file.key);
		SaveFile(Join(destination, file.key), File::Open(input), kBuildPlatformAndroid);
	}
}

bool IsPackage(CString::View value)
{
	if (!value || !Search(value, '.')) return false;
	bool first = true;
	for (auto c : value)
	{
		if (c == '.')
		{
			if (first) return false;
			first = true;
			continue;
		}
		bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
		if ((first && !letter) || (!letter && !(c >= '0' && c <= '9'))) return false;
		first = false;
	}
	return !first;
}

CString::View AbiName(Architecture architecture)
{
	static constexpr CString::View names[] = { {}, "x86", "x86_64", "armeabi-v7a", "arm64-v8a" };
	REFLEX_STATIC_ASSERT(GetArraySize(names) == kArchitectureCount);
	Require(UInt(architecture) < kArchitectureCount, kArchitectures, Join("unsupported architecture: ", ToCString(UInt(architecture))));
	auto result = names[architecture];
	Require(True(result), kArchitectures, Join("unsupported architecture: ", kArchitectureNames[architecture]));
	return result;
}

WString AbiList(const TargetConfiguration & config)
{
	WString result = L"[";
	auto architectures = config.GetArchitectures();
	for (UInt i = 0; i < architectures.GetSize(); ++i)
	{
		auto abi = AbiName(architectures[i]);
		if (i) result.Append(L", ");
		result.Append(Join(WChar(kSingleQuote), ToWString(abi), WChar(kSingleQuote)));
	}
	result.Push(L']');
	return result;
}

WString OutputDirectory(const TargetConfiguration & config)
{
	auto output_directory = config.GetPath(kOutputDirectory);
	if (!output_directory.path) return {};
	return File::CorrectTrailingStroke(ResolvePath(config.platform->target->project->GetRoot(), output_directory));
}

void AddApplicationArtifact(Map<WString> & outputs, const TargetConfiguration & config)
{
	auto directory = OutputDirectory(config);
	if (!directory) return;
	directory = Replace(directory, L"$(Architecture)", L"universal");
	auto filename = Join(AssembledValue(config.GetString(kOutputName, config.platform->target->project->GetName())), ".apk");
	outputs.Set(Join(directory, ToWString(filename)));
}

void AddLibraryArtifacts(Map<WString> & outputs, const BuildTarget & target, const TargetConfiguration & config)
{
	auto directory = OutputDirectory(config);
	if (!directory) return;
	for (auto architecture : config.GetArchitectures())
	{
		auto resolved = Replace(directory, L"$(Architecture)", ToWString(AbiName(architecture)));
		outputs.Set(Join(resolved, L"lib", ToWString(target.output_library), L".a"));
	}
}

Data::Archive Settings(CString::View project_name, CString::View module)
{
	return ProjectGen::Template(settings_gradle,
	{
		{ "PROJECT_NAME", GradleQuote(ToWString(project_name)) },
		{ "MODULE", ToWString(module) },
	});
}

CString PropertiesValue(CString::View value)
{
	CString result;
	for (auto c : value)
	{
		if (c == '\\') result.Append("\\\\");
		else if (c == '\n') result.Append("\\n");
		else if (c == '\r') result.Append("\\r");
		else if (c == '\t') result.Append("\\t");
		else if (c == '\f') result.Append("\\f");
		else
		{
			if (c == ' ' || c == '=' || c == ':' || c == '#' || c == '!') result.Push('\\');
			result.Push(c);
		}
	}
	return result;
}

Data::Archive SdkProperties(CString::View path)
{
	Data::Archive output;
	Data::WriteLine(output, "# Generated Android SDK bridge. Do not commit this file.");
	Data::WriteLine(output, Join("sdk.dir=", PropertiesValue(path)));
	return output;
}

Data::Archive AppGradle(const Target & target, CString::View package_id, const TargetConfiguration & debug, const TargetConfiguration & release)
{
	Variable signing = { "SIGNING" };
	auto write_signing = [&signing](CString::View configuration, CString::View configuration_id, const TargetConfiguration & target_configuration)
	{
		if (auto path = target_configuration.GetPath(kAndroidSigningProperties); path.path)
		{
			signing.value.Append(Text(ProjectGen::Template(Data::Pack(kSigning),
			{
				{ "CONFIGURATION", ToWString(configuration) },
				{ "CONFIGURATION_ID", ToWString(configuration_id) },
				{ "SIGNING_PROPERTIES", GradleQuote(GradlePath(path)) },
			})));
		}
	};
	write_signing("debug", "Debug", debug);
	write_signing("release", "Release", release);

	Data::Archive build_actions;
	WriteBuildActions(build_actions, kDebug, debug);
	WriteBuildActions(build_actions, kRelease, release);
	Data::Archive artifact_outputs;
	WriteArtifactCopy(artifact_outputs, kDebug, debug);
	WriteArtifactCopy(artifact_outputs, kRelease, release);
	Data::Archive dependencies;
	WriteDependencies(dependencies, false, debug);
	WriteDependencies(dependencies, true, release);

	return ProjectGen::Template(app_build_gradle,
	{
		{ "PACKAGE_ID", GradleQuote(ToWString(package_id)) },
		{ "ANDROID_SDK", ToWString(SdkVersion(debug, kAndroidSdk, 37)) },
		{ "ANDROID_MIN_SDK", ToWString(SdkVersion(debug, kAndroidMinSdk, 28)) },
		{ "NATIVE_LIBRARY", ToWString(GradleQuote(Symbol(target.GetName()))) },
		{ "DEBUG_RUNTIME_LIBRARY", RuntimeLibraryName(debug.GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static)) },
		{ "DEBUG_ABIS", AbiList(debug) },
		{ "RELEASE_RUNTIME_LIBRARY", RuntimeLibraryName(release.GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static)) },
		{ "RELEASE_ABIS", AbiList(release) },
		signing,
		{ "BUILD_FEATURES", WString::View(L"\tbuildFeatures {\n\t\tprefab = true\n\t}\n\n") },
		{ "BUILD_ACTIONS", Text(build_actions) },
		{ "ARTIFACT_OUTPUTS", Text(artifact_outputs) },
		{ "DEPENDENCIES", Text(dependencies) },
	});
}

Data::Archive LibraryGradle(ArrayView<BuildTarget> targets, CString::View package_id)
{
	REFLEX_ASSERT(targets.size);
	auto & debug = *targets[0].debug;
	auto & release = *targets[0].release;

	Data::Archive build_actions;
	WriteBuildActions(build_actions, kDebug, debug);
	WriteBuildActions(build_actions, kRelease, release);
	Data::Archive dependencies;
	WriteDependencies(dependencies, false, debug);
	WriteDependencies(dependencies, true, release);
	WString native_targets;
	for (auto & target : targets)
	{
		if (native_targets) native_targets.Append(L", ");
		native_targets.Append(GradleQuote(ToWString(target.cmake_module)));
	}

	return ProjectGen::Template(library_build_gradle,
	{
		{ "PACKAGE_ID", ToWString(GradleQuote(package_id)) },
		{ "ANDROID_SDK", ToWString(SdkVersion(debug, kAndroidSdk, 37)) },
		{ "ANDROID_MIN_SDK", ToWString(SdkVersion(debug, kAndroidMinSdk, 28)) },
		{ "DEBUG_RUNTIME_LIBRARY", RuntimeLibraryName(debug.GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static)) },
		{ "DEBUG_ABIS", AbiList(debug) },
		{ "RELEASE_RUNTIME_LIBRARY", RuntimeLibraryName(release.GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static)) },
		{ "RELEASE_ABIS", AbiList(release) },
		{ "NATIVE_TARGETS", native_targets },
		{ "BUILD_ACTIONS", Text(build_actions) },
		{ "DEPENDENCIES", Text(dependencies) },
	});
}

void WriteCMakeHeaderOnlyFiles(Data::Archive & output, CString::View library, ArrayView<PathDesc> headers, ArrayView<PathDesc> other)
{
	Array<WString> paths;
	for (auto & path : headers) paths.Push(CMakeQuote(Path(path)));
	for (auto & path : other) paths.Push(CMakeQuote(Path(path)));
	if (paths.Empty()) return;
	auto properties = paths;
	properties.Push(L"PROPERTIES HEADER_FILE_ONLY TRUE");
	WriteCMakeInvocation(output, 0, "set_source_files_properties", {}, properties);
	WriteCMakeTargetValues(output, 0, "target_sources", library, paths);
	Data::WriteLine(output);
}

Data::Archive CMakeConfiguration(const TargetConfiguration & config, CString::View library, bool is_release, bool application)
{
	constexpr CString::View standard_versions[] = { "17", "20" };
	REFLEX_STATIC_ASSERT(GetArraySize(standard_versions) == kCppStandardCount);

	auto condition = is_release ? CString::View("CMAKE_BUILD_TYPE STREQUAL \"Release\" OR CMAKE_BUILD_TYPE STREQUAL \"RelWithDebInfo\" OR CMAKE_BUILD_TYPE STREQUAL \"MinSizeRel\"") : CString::View("CMAKE_BUILD_TYPE STREQUAL \"Debug\"");
	Array<WString> sources, include_directories, defines, compile_options, libraries;
	Array<CString> packages;
	auto files = GetFiles(config);
	for (auto & source : FlattenPaths(*files.sources)) sources.Push(CMakeQuote(Path(source)));
	Require(!sources.Empty(), kSources, "are required");
	for (auto & path : FlattenPaths(*config.GetPaths(true, kIncludeDirectories))) include_directories.Push(CMakeQuote(Path(path)));
	for (auto & define : config.GetDefinitions()) defines.Push(CMakeQuote(Variables(ToWString(define))));
	for (auto option : GnuCompileOptions(config, false)) compile_options.Push(ToWString(option));
	for (auto option : ClangFloatingPointOptions(config)) compile_options.Push(ToWString(option));
	for (auto & option : config.GetStrings(kCompilerOptions)) compile_options.Push(CMakeQuote(Variables(ToWString(option))));
	for (auto & dependency : GetLinkDependencies(*config.platform->target, kBuildPlatformAndroid))
	{
		if (!dependency->IsLibrary())
		{
			libraries.Push(ToWString(SourceTargetIdentifier(*dependency)));
			continue;
		}

		auto dependency_platform = dependency->FindPlatform(kBuildPlatformAndroid);
		REFLEX_ASSERT(dependency_platform);
		auto dependency_config = dependency_platform->FindConfiguration(config.GetName());
		REFLEX_ASSERT(dependency_config);
		auto path = dependency_config->GetPath(kPath);
		auto identifier = CMakeIdentifier(*dependency);
		if (path.path)
		{
			libraries.Push(CMakeQuote(Variables(ResolvePath(dependency->project->GetRoot(), path))));
		}
		else if (auto separator = Search(identifier, "::"))
		{
			libraries.Push(ToWString(identifier));
			auto package = CString(Left(identifier, separator.value));
			if (!Search(packages, package)) packages.Push(std::move(package));
		}
		else libraries.Push(ToWString(identifier));
	}
	if (application)
	{
		libraries.Push(L"android");
		libraries.Push(L"log");
		libraries.Push(L"EGL");
		libraries.Push(L"GLESv3");
	}
	Data::Archive output;
	Data::WriteLine(output, Join("if(", condition, ")"));
	for (auto & package : packages) WriteCMakeInvocation(output, 1, "find_package", ToWString(Join(package, " REQUIRED CONFIG")));
	WriteCMakeInvocation(output, 1, "set_property", ToWString(Join("TARGET ", library, " PROPERTY CXX_STANDARD ",
		standard_versions[config.GetEnum<CppStandard>(kCppStandard, kCppStandardNames, kCppStandard_cxx20)])));
	WriteCMakeTargetValues(output, 1, "target_sources", library, sources);
	WriteCMakeTargetValues(output, 1, "target_include_directories", library, include_directories);
	WriteCMakeTargetValues(output, 1, "target_compile_definitions", library, defines);
	WriteCMakeTargetValues(output, 1, "target_compile_options", library, compile_options);
	WriteCMakeTargetValues(output, 1, "target_link_libraries", library, libraries);
	if (!application)
	{
		auto output_directory = config.GetPath(kOutputDirectory);
		if (output_directory.path)
		{
			Require(True(Search(output_directory.path, L"$(Architecture)")), kOutputDirectory,
				"must contain $(Architecture) for Android static libraries");
			auto value = CMakeQuote(Variables(ResolvePath(config.platform->target->project->GetRoot(), output_directory)));
			WriteCMakeInvocation(output, 1, "set_property", ToWString(Join("TARGET ", library, " PROPERTY ARCHIVE_OUTPUT_DIRECTORY")), { value });
		}
	}
	auto optimized = config.GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none) != kOptimization_none;
	if (config.GetBool(kDebugInformation, !optimized)) WriteCMakeTargetValues(output, 1, "target_link_options", library, { L"-g" });
	if (config.GetBool(kDeadStrip, optimized)) WriteCMakeTargetValues(output, 1, "target_link_options", library, { L"\"-Wl,--gc-sections\"" });
	Data::WriteLine(output, "endif()");
	return output;
}

void WriteSourceProjectImports(Data::Archive & output, const Target & target)
{
	Array<const Project *> imported_projects;
	for (auto & dependency : GetLinkDependencies(target, kBuildPlatformAndroid))
	{
		if (dependency->IsLibrary() || dependency->project.Adr() == target.project.Adr()) continue;

		auto dependency_project = dependency->project.Adr();
		if (Search(imported_projects, dependency_project)) continue;
		imported_projects.Push(dependency_project);

		auto identifier = SourceTargetIdentifier(*dependency);
		auto cmake_identifier = CMakeIdentifier(*dependency);
		CString module;
		if (Search(cmake_identifier, "::"))
		{
			CString ignored;
			SplitCMakeIdentifier(cmake_identifier, module, ignored);
		}
		else module = Lowercase(Symbol(dependency_project->GetName()));

		auto source = Join(GetProjectFolder(*dependency_project, kBuildPlatformAndroid), ToWString(module));
		auto binary = Join(L"${CMAKE_BINARY_DIR}/reflex_dependencies/", ToWString(Symbol(dependency_project->GetName())));
		Data::WriteLine(output, Join("if(NOT TARGET ", identifier, ")"));
		WriteLine(output, 1, Join(L"add_subdirectory(", CMakeQuote(source), L" ", CMakeQuote(binary), L")"));
		Data::WriteLine(output, "endif()");
		Data::WriteLine(output, CString::View {});
	}
}

CString WriteCMakeBuildTarget(Data::Archive & output, const BuildTarget & target, bool application)
{
	if (application || target.library == target.cmake_module) return target.library;

	auto variable = Join(target.library, "_BUILD_TARGET");
	Data::WriteLine(output, "if(PROJECT_IS_TOP_LEVEL)");
	WriteCMakeInvocation(output, 1, "set", ToWString(variable), { ToWString(target.cmake_module) });
	Data::WriteLine(output, "else()");
	WriteCMakeInvocation(output, 1, "set", ToWString(variable), { ToWString(target.library) });
	Data::WriteLine(output, "endif()");
	return Join("${", variable, "}");
}

Data::Archive CMake(CString::View project_name, ArrayView<BuildTarget> targets, bool application)
{
	Data::Archive native_app_glue, target_definitions;
	Data::Archive configurations;
	bool has_native_app_glue = false;
	for (auto & target : targets) has_native_app_glue = has_native_app_glue || target.debug->GetBool(kAndroidNativeAppGlue);
	if (has_native_app_glue)
	{
		Data::WriteLine(native_app_glue, "add_library(ReflexAndroidNativeAppGlue OBJECT ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)");
		Data::WriteLine(native_app_glue, "target_compile_definitions(ReflexAndroidNativeAppGlue PRIVATE NDEBUG=1)");
		Data::WriteLine(native_app_glue, CString::View {});
	}
	for (auto & target : targets) WriteSourceProjectImports(target_definitions, *target.target);

	for (auto & target : targets)
	{
		auto build_target = WriteCMakeBuildTarget(target_definitions, target, application);
		Data::WriteLine(target_definitions, Join("add_library(", build_target, application ? " SHARED)" : " STATIC)"));
		if (!application && target.library != target.cmake_module)
		{
			Data::WriteLine(target_definitions, "if(PROJECT_IS_TOP_LEVEL)");
			WriteCMakeInvocation(target_definitions, 1, "add_library", ToWString(Join(target.library, " ALIAS ", target.cmake_module)));
			Data::WriteLine(target_definitions, "endif()");
		}
		if (target.output_library != build_target) Data::WriteLine(target_definitions, Join("set_property(TARGET ", build_target, " PROPERTY OUTPUT_NAME ", target.output_library, ")"));
		if (application) Data::WriteLine(target_definitions, Join("target_link_options(", build_target, " PRIVATE \"-Wl,-u,ANativeActivity_onCreate\")"));
		if (target.debug->GetBool(kAndroidNativeAppGlue))
		{
			Data::WriteLine(target_definitions, Join("target_sources(", build_target, " PRIVATE $<TARGET_OBJECTS:ReflexAndroidNativeAppGlue>)"));
			Data::WriteLine(target_definitions, Join("target_include_directories(", build_target, " PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)"));
		}
		auto files = GetFiles(*target.debug);
		WriteCMakeHeaderOnlyFiles(target_definitions, build_target, FlattenPaths(*files.headers), FlattenPaths(*files.other));
		Data::WriteLine(target_definitions, CString::View {});
		configurations.Append(CMakeConfiguration(*target.debug, build_target, false, application));
		configurations.Append(CMakeConfiguration(*target.release, build_target, true, application));
	}

	return ProjectGen::Template(CMakeLists_txt,
	{
		{ "PROJECT", ToWString(Symbol(project_name)) },
		{ "NATIVE_APP_GLUE", Text(native_app_glue) },
		{ "TARGETS", Text(target_definitions) },
		{ "CONFIGURATIONS", Text(configurations) },
	});
}

Data::Archive Activity(CString::View package_id, CString::View library)
{
	return ProjectGen::Template(MainActivity_kt,
	{
		{ "PACKAGE_ID", ToWString(package_id) },
		{ "NATIVE_LIBRARY", ToWString(library) },
	});
}

void WriteHostHelpers(const WString & directory, const TargetConfiguration & config, CString::View gradle_configuration, bool application)
{
	auto build_name = Join(L"Build ", ToWString(config.GetName()));
	auto task = application ? Join("assemble", gradle_configuration) : Join("externalNativeBuild", gradle_configuration);
	SaveText(Join(directory, build_name, L".bat"), Join("call \"%~dp0gradlew.bat\" ", task, "\r\n"), kBuildPlatformAndroid);
	SaveCommandScript(Join(directory, build_name, L'.', ReflexCLI::kCommand), Join(L"#!/bin/sh\nexec \"$(dirname \"$0\")/gradlew\" ", ToWString(task), L" </dev/null\n"), kBuildPlatformAndroid);
}

void SetExecutable(const WString & path)
{
	constexpr UInt32 kExecuteBits = 0111;
	UInt32 permissions;
	bool read = GetFilePermissions(path, permissions);
	Require(read && SetFilePermissions(path, permissions | kExecuteBits), "gradlew", "error applying permissions");
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::GenerateAndroidProject(Project & project, BuildPlatform, const WString & directory, Array<const TargetPlatform *> & generated_platforms, Map<WString> & outputs)
{
	Array<Android::BuildTarget> targets;
	OutputType output_type = kOutputTypeCount;
	for (auto & planned : CollectTargetPlatforms(project, kBuildPlatformAndroid, generated_platforms))
	{
		auto & item = *planned.target;
		if (item.project.Adr() != &project || item.IsLibrary()) continue;
		auto platform = planned.platform;
		Require(platform->GetTargetConfigurations().size == 2, kConfigurations, "Debug and Release are required");
		auto [debug,release] = FindDebugAndReleaseConfigurations(*platform);
		Require(True(debug) && True(release), kConfigurations, "Debug and Release are required");

		auto target_type = GetOutputType(*platform);
		Require(target_type == kOutputType_app || target_type == kOutputType_static_library, kOutputType, Join("unsupported: ", item.GetName()));
		if (output_type == kOutputTypeCount) output_type = target_type;
		Require(target_type == output_type, kOutputType, "app and static_library targets cannot be mixed");

		const TargetConfiguration * configurations[] = { debug, release };
		for (auto config : configurations)
		{
			Require(config->GetBuildActions(kBuildPhasePostBuild, System::kPlatformAndroid).Empty(), kBuildPhases[kBuildPhasePostBuild], "actions are not supported");
			Require(config->GetString(kOutputExtension).Empty(), kOutputExtension, "is not supported");
		}
		GetConfigurationInvariant(*platform, kAndroidNativeAppGlue, [](const TargetConfiguration & configuration) { return configuration.GetBool(kAndroidNativeAppGlue); });
		GetConfigurationInvariant(*platform, kOtherFiles,
			[](const TargetConfiguration & configuration) { return FlattenPaths(*GetFiles(configuration).other); },
			[](ArrayView<PathDesc> a, ArrayView<PathDesc> b) { return ComparePaths(a, b); });

		Android::BuildTarget target;
		target.target = &item;
		target.platform = platform;
		target.debug = debug;
		target.release = release;
		target.library = Android::Symbol(item.GetName());
		target.output_library = target.library;
		target.cmake_identifier = Android::CMakeIdentifier(item);
		targets.Push(std::move(target));
	}
	if (targets.Empty()) return;
	bool application = output_type == kOutputType_app;
	Require(!application || targets.GetSize() == 1, kTargets, "app projects support exactly one target");
	if (!application)
	{
		for (auto & target : targets)
		{
			Require(target.debug->GetPath(kAndroidSigningProperties).path.Empty() && target.release->GetPath(kAndroidSigningProperties).path.Empty(),
				kAndroidSigningProperties, "is only supported for app targets");
		}
	}
	auto & main = targets.GetFirst();
	auto debug = main.debug;
	auto release = main.release;
	bool default_configuration_is_release = (release->GetName() == project.GetDefaultConfiguration());
	CString::View default_configuration = default_configuration_is_release ? kRelease : kDebug;

	auto debug_sdk = GetConfigurationInvariant(*main.platform, kAndroidSdk,
		[](const TargetConfiguration & configuration) { return Android::SdkVersion(configuration, kAndroidSdk, 37); });
	auto debug_min_sdk = GetConfigurationInvariant(*main.platform, kAndroidMinSdk,
		[](const TargetConfiguration & configuration) { return Android::SdkVersion(configuration, kAndroidMinSdk, 28); });
	Require(debug_sdk && debug_min_sdk, kAndroidSdk, "and min_sdk must be greater than zero");
	Require(debug_min_sdk <= debug_sdk, kAndroidMinSdk, "must not exceed sdk");
	auto package_id = GetConfigurationInvariant(*main.platform, kAndroidPackageId,
		[](const TargetConfiguration & configuration) { return configuration.GetString(kAndroidPackageId); });
	Require(Android::IsPackage(package_id), kAndroidPackageId, "must be a valid Java package name");
	auto main_source_sets = GetConfigurationInvariant(*main.platform, kAndroidMainSourceSet,
		[](const TargetConfiguration & configuration) { return configuration.GetAppendableStrings(kAndroidMainSourceSet); });
	for (auto & target : targets)
	{
		const TargetConfiguration * configurations[] = { target.debug, target.release };
		const TargetConfiguration * references[] = { debug, release };
		for (UInt index = 0; index < 2; ++index)
		{
			auto config = configurations[index];
			auto reference = references[index];
			Require(Android::SdkVersion(*config, kAndroidSdk, 37) == Android::SdkVersion(*reference, kAndroidSdk, 37) && Android::SdkVersion(*config, kAndroidMinSdk, 28) == Android::SdkVersion(*reference, kAndroidMinSdk, 28), kAndroidSdk, "versions must be identical across targets");
			Require(config->GetString(kAndroidPackageId) == package_id, kAndroidPackageId, "must be identical across targets");
			Require(config->GetAppendableStrings(kAndroidMainSourceSet) == main_source_sets, kAndroidMainSourceSet, "must be identical across targets");
			Require(Android::AbiList(*config) == Android::AbiList(*reference), kArchitectures, "must be identical across targets");
			Require(config->GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static) == reference->GetEnum<RuntimeLibrary>(kRuntimeLibrary, kRuntimeLibraryNames, kRuntimeLibrary_static), kRuntimeLibrary, "must be identical across targets");
		}
	}
	Array<WString> resolved_main_source_sets;
	for (auto & value : main_source_sets)
	{
		auto path = ResolvePath(project.GetRoot(), DecodePath(value));
		File::Detail::CorrectTrailingStroke(path);
		Require(File::IsDirectory(path), kAndroidMainSourceSet, Join("folder not found: ", EncodeUTF8(path)));
		resolved_main_source_sets.Push(std::move(path));
	}
	{
		auto listing_archive = File::Extract(Android::listing_txt);
		auto listing = Data::Unpack<CString::View>(listing_archive);
		for (auto file : Split(listing, '\n'))
		{
			if (!file) continue;
			if (!application && Left(file, 4) == "app/") continue;
			auto resource = File::EnumerableEmbeddedResource::Retrieve({ K32("ReflexCLI::ProjectGen::Android"), MakeKey32(file) });
			REFLEX_ASSERT(resource);
			SaveFile(Join(directory, ToWString(file)), File::Extract(*resource), kBuildPlatformAndroid);
		}
	}
	Android::SetExecutable(Join(directory, L"gradlew"));
	auto package_path = Replace(package_id, ".", "/");
	if (!application)
	{
		CString project_cmake_package;
		for (auto & target : targets)
		{
			if (Search(target.cmake_identifier, "::")) Android::SplitCMakeIdentifier(target.cmake_identifier, target.cmake_package, target.cmake_module);
			else
			{
				target.cmake_package = Lowercase(Android::Symbol(project.GetName()));
				target.cmake_module = target.library;
			}
			target.library = Android::SourceTargetIdentifier(*target.target);
			if (!project_cmake_package) project_cmake_package = target.cmake_package;
			else Require(target.cmake_package == project_cmake_package, kCMakeIdentifier, "all Android library targets must use the same CMake namespace");
		}
		main.cmake_package = std::move(project_cmake_package);
	}
	auto module = application ? CString("app") : CString(main.cmake_package);

	if (auto path = debug->GetString(kAndroidSdkPath)) SaveFile(Join(directory, L"local.properties"), Android::SdkProperties(path), kBuildPlatformAndroid);
	SaveFile(Join(directory, L"build.gradle"), Template(Android::build_gradle, { { "ANDROID_PLUGIN", application ? WString::View(L"application") : WString::View(L"library") }, { "DEFAULT_CONFIGURATION", ToWString(default_configuration) } }), kBuildPlatformAndroid);
	SaveFile(Join(directory, L"settings.gradle"), Android::Settings(project.GetName(), module), kBuildPlatformAndroid);
	SaveFile(Join(directory, ToWString(module), L"/build.gradle"), application ? Android::AppGradle(*targets.GetFirst().target, package_id, *debug, *release) : Android::LibraryGradle(targets, package_id), kBuildPlatformAndroid);
	SaveFile(Join(directory, ToWString(module), L"/CMakeLists.txt"), Android::CMake(project.GetName(), targets, application), kBuildPlatformAndroid);
	Android::WriteHostHelpers(directory, *debug, kDebug, application);
	Android::WriteHostHelpers(directory, *release, kRelease, application);
	if (application)
	{
		Android::AddApplicationArtifact(outputs, *debug);
		Android::AddApplicationArtifact(outputs, *release);
	}
	else for (auto & target : targets)
	{
		Android::AddLibraryArtifacts(outputs, target, *target.debug);
		Android::AddLibraryArtifacts(outputs, target, *target.release);
	}
	if (application)
	{
		SaveFile(Join(directory, ToWString(module), L"/src/main/java/", ToWString(package_path), L"/MainActivity.kt"), Android::Activity(package_id, targets.GetFirst().library), kBuildPlatformAndroid);
		SaveText(Join(directory, ToWString(module), L"/src/main/res/values/strings.xml"), Join("<resources>\n\t<string name=\"app_name\">", EscapeXml(Android::AssembledValue(debug->GetString(kOutputName, debug->platform->target->project->GetName()))), "</string>\n</resources>\n"), kBuildPlatformAndroid);
	}

	for (auto & source : resolved_main_source_sets) Android::OverlaySourceSet(source, Join(directory, ToWString(module), L"/src/main/"));
}
