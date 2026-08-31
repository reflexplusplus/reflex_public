#pragma once

#include "project_gen_schema.h"
#include "project_gen_preprocessor.h"

namespace ReflexCLI::ProjectGen
{
	using namespace Reflex;

	constexpr CString::View kFormat = "format";
	constexpr CString::View kProjectInclude = "include";
	constexpr CString::View kProjectName = "name";
	constexpr CString::View kGeneratedDirectory = "generated_directory";
	constexpr CString::View kDefaultConfiguration = "default_configuration";
	//constexpr CString::View kVariables = "variables";
	constexpr CString::View kOutputName = "output_name";
	constexpr CString::View kOutputType = "type";
	constexpr CString::View kOutputExtension = "extension";
	constexpr CString::View kArchitectures = "architectures";
	constexpr CString::View kSources = "sources";
	constexpr CString::View kHeaders = "headers";
	constexpr CString::View kOtherFiles = "other";
	constexpr CString::View kWindowsResources = "resources";
	constexpr CString::View kAndroidArchiveName = "archive_name";
	constexpr CString::View kAndroidNativeAppGlue = "native_app_glue";
	constexpr CString::View kAndroidSdkPath = "sdk_path";
	constexpr CString::View kAndroidDependencies = "gradle_dependencies";
	constexpr CString::View kAndroidSdk = "sdk";
	constexpr CString::View kAndroidMinSdk = "min_sdk";
	constexpr CString::View kAndroidPackageId = "package_id";
	constexpr CString::View kAndroidMainSourceSet = "main_source_set";
	constexpr CString::View kAndroidSigningProperties = "signing_properties";
	constexpr CString::View kOutputDirectory = "output_directory";
	constexpr CString::View kIntermediateDirectory = "intermediate_directory";
	constexpr CString::View kCppStandard = "cpp_standard";
	constexpr CString::View kRtti = "rtti";
	constexpr CString::View kDefines = "defines";
	constexpr CString::View kIncludeDirectories = "include_directories";
	constexpr CString::View kPublicIncludeDirectories = "public_include_directories";
	constexpr CString::View kCompilerOptions = "compiler_options";
	constexpr CString::View kWarningLevel = "warning_level";
	constexpr CString::View kOptimization = "optimization";
	constexpr CString::View kFloatingPoint = "floating_point";
	constexpr CString::View kRuntimeLibrary = "runtime_library";
	constexpr CString::View kDebugInformation = "debug_information";
	constexpr CString::View kDeadStrip = "dead_strip";
	constexpr CString::View kMacOSExportedSymbols = "exported_symbols";
	constexpr CString::View kMacOSBuildAllArchitectures = "build_all_architectures";
	constexpr CString::View kWindowsSdk = "sdk";
	constexpr CString::View kCMakeIdentifier = "cmake_identifier";
	constexpr CString::View kCMakeFindPackages = "cmake_find_packages";
	constexpr CString::View kCMakeIncludes = "cmake_includes";
	constexpr CString::View kCMakeVariables = "cmake_variables";
	constexpr CString::View kXcodeFrameworkProperties[2] = { "frameworks", "frameworks" };
	constexpr CString::View kXcodeArcProperties[2] = { "arc", "arc" };
	constexpr CString::View kXcodeBundleIdentifierProperties[2] = { "bundle_identifier", "bundle_identifier" };
	constexpr CString::View kXcodeCodesignProperties[2] = { "code_sign", "code_sign" };
	constexpr CString::View kXcodeDevelopmentTeamProperties[2] = { "development_team", "development_team" };
	constexpr CString::View kXcodeInfoPlistProperties[2] = { "info_plist", "info_plist" };
	constexpr CString::View kXcodeEntitlementsProperties[2] = { "entitlements", "entitlements" };
	constexpr CString::View kXcodeIconProperties[2] = { "icon", "icon" };
	constexpr CString::View kXcodeMinimumVersionProperties[2] = { "deployment_target", "deployment_target" };
	constexpr CString::View kXcodeLaunchScreenProperty = "launch_screen";
	constexpr CString::View kName = "name";
	constexpr CString::View kPath = "path";
	constexpr CString::View kCommand = "command";
	constexpr CString::View kImport = "import";
	constexpr CString::View kInputs = "inputs";
	constexpr CString::View kOutputs = "outputs";
	constexpr CString::View kAlwaysRun = "always_run";
	constexpr CString::View kConfigurations = "configurations";
	constexpr CString::View kTargets = "targets";
	constexpr CString::View kDependencies = "dependencies";
	constexpr CString::View kFiles = "files";
	constexpr CString::View kBuildPhases[3] = {"post_generate", "pre_build", "post_build"};

	DECLARE_ENUM(OutputType, console, app, dynamic_library, static_library);
	DECLARE_ENUM(WarningLevel, none, relaxed, standard, pedantic);
	DECLARE_ENUM(CppStandard, cxx17, cxx20);
	DECLARE_ENUM(Optimization, none, size, speed, full);
	DECLARE_ENUM(FloatingPoint, default, precise, fast, strict);
	DECLARE_ENUM(RuntimeLibrary, static, dynamic);
	DECLARE_ENUM(Architecture, native, x86, x64, arm32, arm64);

	constexpr CString::View kDebug = "Debug";
	constexpr CString::View kRelease = "Release";


	//elements
	
	struct PathDesc
	{
		bool operator==(const PathDesc & b) const = default;

		CString source;
		WString path;
		bool is_absolute = false;
	};

	struct PathGroup : public Node <PathGroup>
	{
		static PathGroup & null;

		using Node<PathGroup>::Attach;

		CString name;
		Array<Pair<PathDesc, Key32>> paths;
	};

	struct FileGroups
	{
		Reference<PathGroup> sources;
		Reference<PathGroup> headers;
		Reference<PathGroup> other;
	};

	enum BuildPhase : UInt8
	{
		kBuildPhasePostGenerate,
		kBuildPhasePreBuild,
		kBuildPhasePostBuild
	};

	struct BuildActionDesc
	{
		bool operator==(const BuildActionDesc & b) const { return command == b.command; }

		CString name;
		Array<WString> command;
		Array<PathDesc> inputs, outputs;
		bool always_run = false;
	};


	//project structure entities

	class CompiledObject : public Object
	{
	public:
		CString::View GetName() const { return m_name; }

	protected:
		CompiledObject(CString::View name = {}) : m_name(name) {}
		CString m_name;
	};

	class Target;
	class TargetPlatform;
	class TargetConfiguration;

	class Project : public ValidatedPropertySet
	{
	public:
		static Project & null;

		Project() = default;
		Project(const Data::KeyMap & keymap);
		static Reference<Project> Acquire(Array <Reference<Project>> & projects, WString::View cfg_path, const Data::KeyMap * keymap = nullptr);

		CString::View GetName() const { return m_name; }
		UInt32 GetFormat() const { return m_format; }
		WString::View GetCfgPath() const { return m_cfg_path; }
		WString::View GetGeneratedDirectory() const { return m_generated_directory; }
		CString::View GetDefaultConfiguration() const { return m_default_configuration; }
		bool IsLibraryOnly() const { return m_library_only; }
		WString::View GetRoot() const { return m_root; }
		ArrayView<Reference<Project>> GetIncludes() const { return m_includes; }
		ArrayView<Reference<Target>> GetTargets() const { return m_targets; }
		void AddInclude(Reference<Project> project);
		const ValidatedPropertySet * FindTemplate(Key32 id) const;
		Reference<Target> FindTarget(CString::View name) const;
		Array<Variable> GetDocumentVariables(Key32 property) const;

	private:
		void Initialize(WString::View cfg_path);
		bool HasDocumentProperty(Key32 id) const;
		void OnQueryProperty(Address address, Object * & object) const override;

		CString m_name;
		UInt32 m_format = 0;
		WString m_cfg_path;
		WString m_generated_directory = L"projects/";
		CString m_default_configuration;
		bool m_library_only = false;
		WString m_root;
		Array<Reference<Project>> m_includes;
		Array<Reference<Target>> m_targets;
		bool m_initialized = false;
	};

	class Target : public CompiledObject
	{
	public:
		static Target & null;

		Target() {}
		Target(Project & project, ValidatedPropertySet & source, bool library);

		const TRef<Project> project;

		bool IsLibrary() const { return m_library; }
		ArrayView<Reference<Target>> GetDependencies() const { return m_dependencies; }
		ArrayView<Reference<Target>> GetDependencies(BuildPlatform platform) const;
		ArrayView<Reference<TargetPlatform>> GetTargetPlatforms() const { return m_platforms; }
		const TargetPlatform * FindPlatform(BuildPlatform platform) const;

		void ResolveDependencies();
		void ResolveIncludeDirectories();

	private:
		bool m_library = false;
		const TRef<ValidatedPropertySet> m_source;
		Array<Reference<Target>> m_dependencies;
		Array<Reference<TargetPlatform>> m_platforms;
	};

	class TargetPlatform : public CompiledObject
	{
	public:
		TargetPlatform(Target & target, PlatformPropertySet & values);

		const TRef<Target> target;

		BuildPlatform GetPlatform() const { return m_platform; }
		ArrayView<Reference<Target>> GetDependencies() const { return m_dependencies; }
		ArrayView<Reference<TargetConfiguration>> GetTargetConfigurations() const { return m_configurations; }
		const TargetConfiguration * FindConfiguration(CString::View name) const;

	private:
		BuildPlatform m_platform = kBuildPlatformWindows;
		const TRef<PlatformPropertySet> m_source;
		Array<CString> m_dependency_names;
		Array<Reference<Target>> m_dependencies;
		Array<Reference<TargetConfiguration>> m_configurations;

		friend class Target;
	};

	class TargetConfiguration : public CompiledObject
	{
	public:
		TargetConfiguration(TargetPlatform & platform, ValidatedPropertySet & values);

		CString GetString(Key32 id, CString::View fallback = {}) const;
		Array<CString> GetStrings(CString::View id) const;
		Array<CString> GetDefinitions() const;
		Array<Variable> GetVariableMap(CString::View id) const;
		PathDesc GetPath(Key32 id) const;
		Reference<PathGroup> GetPaths(bool folders, CString::View property) const;
		bool GetBool(CString::View property, bool fallback = false) const;
		Float32 GetNumber(Key32 id, Float32 fallback = 0.0f) const;

		template <class ENUM> ENUM GetEnum(CString::View property, ArrayView<CString::View> names, ENUM fallback) const
		{
			return ENUM(GetEnumIndex(property, names, UInt(fallback)));
		}

		Array <Architecture> GetArchitectures() const;
		Array <BuildActionDesc> GetBuildActions(BuildPhase phase, System::Platform emission_platform) const;
		OutputType GetProductType() const;
		CString GetVariable(CString::View variable, CString::View fallback = {}) const;
		const PathGroup & GetPublicIncludeDirectories() const;
		void ResolveIncludeDirectories(ArrayView<const TargetConfiguration *> dependencies);

		const TRef <TargetPlatform> platform;


	private:
		Reference<PathGroup> GetDeclaredPaths(bool folders, CString::View property) const;
		WString Expand(WString::View value, System::Platform emission_platform = System::kNumPlatform, bool allow_deferred = true) const;
		CString Expand(CString::View value, System::Platform emission_platform = System::kNumPlatform, bool allow_deferred = true) const;
		UInt GetEnumIndex(CString::View property, ArrayView<CString::View> names, UInt fallback) const;

		const TRef<ValidatedPropertySet> m_source;
		Array<Variable> m_variables;
		Reference<PathGroup> m_include_directories;
		Reference<PathGroup> m_public_include_directories;
	};

	struct PlatformTarget
	{
		Reference<Target> target;
		const TargetPlatform * platform = nullptr;
		Array<Reference<Target>> link_dependencies;
	};

	template <class GETTER, class EQUAL> auto GetConfigurationInvariant(const TargetPlatform & platform, CString::View property, const GETTER & getter, const EQUAL & equal)
	{
		auto configurations = platform.GetTargetConfigurations();
		Require(True(configurations), "target", "has no configurations");
		auto value = getter(*configurations.GetFirst());
		for (auto & configuration : configurations)
		{
			Require(equal(getter(*configuration), value), property, "must be identical across configurations");
		}
		return value;
	}

	template <class GETTER> auto GetConfigurationInvariant(const TargetPlatform & platform, CString::View property, const GETTER & getter)
	{
		return GetConfigurationInvariant(platform, property, getter, [](const auto & a, const auto & b) { return a == b; });
	}

	Array<PlatformTarget> CollectTargetPlatforms(Project & root, BuildPlatform platform, Array<const TargetPlatform *> & generated_platforms);

	Pair <const TargetConfiguration*,const TargetConfiguration*> FindDebugAndReleaseConfigurations(const TargetPlatform & platform);
	OutputType GetOutputType(const TargetPlatform & platform);
	Array<Reference<Target>> GetLinkDependencies(const Target & target, BuildPlatform platform);

	PathDesc DecodePath(CString::View value);
	WString ResolvePath(WString::View root, const PathDesc & path);
	TRef <PathGroup> AcquirePathGroup(PathGroup & parent, CString::View value);
	Array <PathDesc> FlattenPaths(const PathGroup & root);
	Reference <PathGroup> ConsolidatePathGroups(const TargetPlatform & platform);
	FileGroups GetFiles(const TargetConfiguration & config);
	bool ComparePath(const PathDesc & a, const PathDesc & b);
	bool ComparePaths(ArrayView<PathDesc> a, ArrayView<PathDesc> b);

	void GenerateVisualStudioProject(Project &, BuildPlatform, Array<const TargetPlatform *> &);
	void GenerateXcodeProject(Project &, BuildPlatform, Array<const TargetPlatform *> &);
	void GenerateAndroidProject(Project &, BuildPlatform, Array<const TargetPlatform *> &);
	void GenerateLinuxProject(Project &, BuildPlatform, Array<const TargetPlatform *> &);
	void GenerateCMakeProject(Project &, BuildPlatform, Array<const TargetPlatform *> &);

	WString ExpandVariables(WString::View value, ArrayView <Variable> variables, System::Platform platform = System::kNumPlatform);

}





//
//

inline ReflexCLI::ProjectGen::PathDesc ReflexCLI::ProjectGen::TargetConfiguration::GetPath(Key32 property) const
{
	return DecodePath(GetString(property));
}

inline bool ReflexCLI::ProjectGen::TargetConfiguration::GetBool(CString::View property, bool fallback) const
{
	return Data::GetBool(*m_source, property, fallback);
}
