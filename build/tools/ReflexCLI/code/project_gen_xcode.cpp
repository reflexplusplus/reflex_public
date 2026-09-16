#include "project_gen_emitter.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen::Xcode)

constexpr CString::View kMacOSBuild =
"#!/bin/sh\n"
"exec xcodebuild -quiet -workspace [WORKSPACE] -scheme Build -configuration [CONFIGURATION] -destination 'generic/platform=macOS' build\n";

constexpr CString::View kIOSBuild =
"#!/bin/sh\n"
"set -e\n"
"xcodebuild -workspace [WORKSPACE] -scheme Build -configuration [CONFIGURATION] -destination 'generic/platform=iOS' build\n"
"xcodebuild -quiet -workspace [WORKSPACE] -scheme Build -configuration [CONFIGURATION] -destination 'generic/platform=iOS Simulator' build\n";

struct Context
{
	const Project & project;
	const Target & target;
	const TargetPlatform & platform;
	WString directory;
	Array<Target *> dependencies;
	Array<Target *> link_dependencies;
};

REFLEX_STATIC_ASSERT(GetArraySize(kXcodeFrameworkProperties) == 2 && GetArraySize(kXcodeArcProperties) == 2 && GetArraySize(kXcodeBundleIdentifierProperties) == 2 && GetArraySize(kXcodeCodesignProperties) == 2 && GetArraySize(kXcodeDevelopmentTeamProperties) == 2 && GetArraySize(kXcodeInfoPlistProperties) == 2 && GetArraySize(kXcodeEntitlementsProperties) == 2 && GetArraySize(kXcodeIconProperties) == 2 && GetArraySize(kXcodeMinimumVersionProperties) == 2);

UInt PlatformIndex(BuildPlatform platform)
{
	Require(platform == kBuildPlatformMacOS || platform == kBuildPlatformIOS, "platform", "expected macos or ios platform");
	return platform == kBuildPlatformIOS;
}

bool IsBundle(const TargetConfiguration & config)
{
	return config.platform->GetPlatform() == kBuildPlatformMacOS && True(config.GetString(kXcodeBundleIdentifierProperties[0]));
}

class PbxScope
{
public:

	REFLEX_NONCOPYABLE(PbxScope);

	PbxScope(Data::Archive & output, CString::View name) : m_output(output), m_name(name)
	{
		WriteLine(m_output, 0, Join("/* Begin ", m_name, " section */"));
	}

	~PbxScope()
	{
		WriteLine(m_output, 0, Join("/* End ", m_name, " section */"));
		WriteLine(m_output, 0);
	}

private:
	Data::Archive & m_output;
	CString m_name;
};

class PbxObject
{
public:

	REFLEX_NONCOPYABLE(PbxObject);

	PbxObject(Data::Archive & output, CString::View id, CString::View comment)
		: m_output(output)
	{
		WriteLine(m_output, 2, Join(id, comment ? Join(" /* ", comment, " */") : CString {}, " = {"));
	}

	~PbxObject()
	{
		WriteLine(m_output, 2, "};");
	}

	void WriteSetting(CString::View name, CString::View value)
	{
		WriteLine(m_output, 3, Join(name, " = ", value, ';'));
	}

	void WriteSetting(CString::View name, WString::View value)
	{
		WriteLine(m_output, 3, Join(ToWString(name), L" = ", value, L';'));
	}


private:
	Data::Archive & m_output;
};

REFLEX_NOINLINE CString GenerateID(CString::View first, ArrayView <CString::View> middle, CString::View last)
{
	auto combined = Join(first, ':', Merge(middle, ':'), ':', last);

	auto hex = Data::BytesToHex(Data::Pack(Data::FNV1a64(Data::Pack(combined))));

	return Join("A1B2C3D4", hex);
}

CString MakeID(const Context & context, CString::View type)
{
	return GenerateID(context.project.GetName(), { kBuildPlatforms[context.platform.GetPlatform()], type }, context.target.GetName());
}

CString MakeDependencyID(const Context & context, const Target & dependency, CString::View type)
{
	return GenerateID(context.target.GetName(), { "dependency", type, dependency.project->GetName() }, dependency.GetName());
}

CString TargetID(const Target & target, BuildPlatform platform)
{
	return GenerateID(target.project->GetName(), { kBuildPlatforms[platform], "target" }, target.GetName());
}

CString ProductID(const Target & target, BuildPlatform platform)
{
	return GenerateID(target.project->GetName(), { kBuildPlatforms[platform], "product" }, target.GetName());
}

WString EscapePbx(WString::View value)
{
	return Join(L'"', EscapeQuotedString(value), L'"');
}

CString EscapePbx(CString::View value)
{
	return EncodeUTF8(EscapePbx(ToWString(value)));
}

WString TranslateVariables(WString::View value)
{
	auto result = ProjectGen::TranslateVariables(value,
	{
		{ "Architecture", L"$(CURRENT_ARCH)" },
		{ "PlatformVariant", L"$(PLATFORM_NAME)" },
	});

	return result;
}

WString ProjectPath(const Context & context, const PathDesc & path)
{
	if (path.path.Empty()) return {};
	auto value = TranslateVariables(ResolvePath(context.project.GetRoot(), path));
	if (path.is_absolute) return value;
	return File::MakeRelativePath(context.directory, value);
}

WString RootPath(const PathDesc & path)
{
	if (path.path.Empty()) return {};
	return TranslateVariables(ResolvePath(L"$(PROJECT_ROOT)/", path));
}

WString ShellExpandQuote(WString::View value)
{
	WString result;
	UInt offset = 0;
	for (UInt i = 0; i + 2 < value.size; ++i)
	{
		if (value[i] != '$' || value[i + 1] != '(') continue;
		UInt end = i + 2;
		while (end < value.size && value[end] != ')') ++end;
		if (end == value.size || end == i + 2) continue;
		if (i != offset) result.Append(ShellQuote(Mid(value, offset, i - offset)));
		result.Append(Join(L"\"${", Mid(value, i + 2, end - i - 2), L"}\""));
		offset = end + 1;
		i = end;
	}
	if (offset != value.size) result.Append(ShellQuote(Mid(value, offset)));
	return result ? result : WString(L"''");
}

WString ShellCommand(const BuildActionDesc & action)
{
	WString result = L"cd \"${PROJECT_ROOT}\" && ";
	for (auto & argument : action.command)
	{
		result.Append(ShellExpandQuote(TranslateVariables(argument)));
		result.Push(L' ');
	}
	result.Pop();
	return result;
}

bool CompareBuildActions(const BuildActionDesc & a, const BuildActionDesc & b)
{
	if (a.name != b.name || a.always_run != b.always_run || a.command.GetSize() != b.command.GetSize()) return false;
	REFLEX_LOOP(idx, a.command.GetSize()) if (a.command[idx] != b.command[idx]) return false;
	return ComparePaths(a.inputs, b.inputs) && ComparePaths(a.outputs, b.outputs);
}

CString::View FileType(const PathDesc & path)
{
	auto ext = Lowercase(File::SplitExtension(path.path).b);

	switch (MakeKey32(ext))
	{
	case K32("c"): return "sourcecode.c.c";
	case K32("cc"):
	case K32("cpp"):
	case K32("cxx"): return "sourcecode.cpp.cpp";
	case K32("m"): return "sourcecode.c.objc";
	case K32("mm"): return "sourcecode.cpp.objcpp";
	case K32("swift"): return "sourcecode.swift";
	case K32("metal"): return "sourcecode.metal";
	case K32("h"):
	case K32("hpp"):
	case K32("hxx"): return "sourcecode.c.h";
	case K32("xcassets"): return "folder.assetcatalog";
	case K32("storyboard"): return "file.storyboard";
	case K32("xib"): return "file.xib";
	case K32("plist"): return "text.plist.xml";
	case K32("entitlements"): return "text.plist.entitlements";
	case K32("xcconfig"): return "text.xcconfig";
	case K32("strings"): return "text.plist.strings";
	default: return "file";
	}
}

CString Filename(const PathDesc & path)
{
	return EncodeUTF8(File::SplitFilename(path.path).b);
}

bool ContainsPath(ArrayView<PathDesc> paths, const PathDesc & path)
{
	for (auto & value : paths) if (ComparePath(value, path)) return true;
	return false;
}

Array<PathDesc> ResourcePaths(const TargetConfiguration & config)
{
	Array<PathDesc> result;
	auto build_platform = config.platform->GetPlatform();
	auto platform_index = PlatformIndex(build_platform);
	auto icon = config.GetPath(kXcodeIconProperties[platform_index]);
	if (icon.path) result.Push(std::move(icon));
	if (build_platform == kBuildPlatformIOS)
	{
		auto launch_screen = config.GetPath(kXcodeLaunchScreenProperty);
		if (launch_screen.path) result.Push(std::move(launch_screen));
	}
	return result;
}

Array<PathDesc> CollectPaths(const TargetPlatform & platform, UInt kind)
{
	Array<PathDesc> result;
	Array <PathDesc> paths;

	for (auto & config : platform.GetTargetConfigurations())
	{
		auto files = GetFiles(config);

		switch (kind)
		{
		case 0:
			paths = FlattenPaths(*files.sources);
			break;

		case 1:
			paths = FlattenPaths(*files.headers);
			break;

		case 2:
			paths = FlattenPaths(*files.other);
			break;

		default:
			paths = ResourcePaths(config);
			break;
		}
		
		for (auto & path : paths) if (!ContainsPath(result, path)) result.Push(path);
	}

	return result;
}

Array<CString> CollectFrameworks(const TargetPlatform & platform)
{
	Array<CString> result;
	auto platform_index = PlatformIndex(platform.GetPlatform());
	for (auto & config : platform.GetTargetConfigurations())
	{
		for (auto & framework : config->GetAppendableStrings(kXcodeFrameworkProperties[platform_index])) if (!Search(result, framework)) result.Push(framework);
	}
	return result;
}

CString ProductType(const TargetConfiguration & config, BuildPlatform platform, bool bundle)
{
	auto output_type = config.GetProductType();
	Require(platform != kBuildPlatformIOS || output_type == kOutputType_app || output_type == kOutputType_static_library, kOutputType, "only app and static_library targets are supported on iOS");

	CString::View types[] = { "com.apple.product-type.tool", "com.apple.product-type.application", bundle ? "com.apple.product-type.bundle" : "com.apple.product-type.library.dynamic", "com.apple.product-type.library.static" };

	return types[output_type];
}

CString ProductExtension(const TargetConfiguration & config, bool bundle)
{
	if (auto output_extension = config.GetString(kOutputExtension))
	{
		return EncodeUTF8(TranslateVariables(ToWString(output_extension)));
	}
	else
	{
		switch (config.GetProductType())
		{
		case kOutputType_app: return "app";
		case kOutputType_dynamic_library: return bundle ? "bundle" : "dylib";
		case kOutputType_static_library: return "a";
		default: return {};
		}
	}
}

CString ProductFilename(const TargetConfiguration & config, bool bundle)
{
	auto extension = ProductExtension(config, bundle);
	auto output_type = config.GetProductType();
	auto output_name = EncodeUTF8(TranslateVariables(ToWString(config.GetString(kOutputName, config.platform->target->project->GetName()))));
	if (output_type == kOutputType_static_library || (output_type == kOutputType_dynamic_library && !bundle))
	{
		return Join("lib", output_name, extension ? Join(".", extension) : CString {});
	}
	return Join(output_name, extension ? Join(".", extension) : CString {});
}

WString DependencyLibraryPath(const Context & context, const Target & dependency, CString::View configuration)
{
	auto platform = dependency.FindPlatform(context.platform.GetPlatform());
	REFLEX_ASSERT(platform);
	auto config = platform->FindConfiguration(configuration);
	REFLEX_ASSERT(config);
	auto path = config->GetPath(kPath);
	if (!path.path) return ToWString(Join("-l", dependency.GetName()));
	return TranslateVariables(ResolvePath(dependency.project->GetRoot(), path));
}

CString::View ArchitectureName(Architecture architecture)
{
	constexpr CString::View names[] = { {}, {}, "x86_64", {}, "arm64" };
	static_assert(GetArraySize(names) == kArchitectureCount);
	Require(UInt(architecture) < kArchitectureCount, "architectures", Join("unsupported architecture: ", ToCString(UInt(architecture))));
	Require(True(names[architecture]), "architectures", Join("unsupported architecture: ", kArchitectureNames[architecture]));
	return names[architecture];
}

Array<CString> Architectures(const TargetConfiguration & config)
{
	Array<CString> result;
	for (auto architecture : config.GetArchitectures()) result.Push(ArchitectureName(architecture));
	return result;
}

void AddOutputs(Map<WString> & outputs, const Project & project, ArrayView<Context> contexts)
{
	for (auto & context : contexts)
	{
		if (context.target.project.Adr() != &project) continue;
		const CString::View macos_variants[] = { "macosx" };
		const CString::View ios_variants[] = { "iphoneos", "iphonesimulator" };
		auto variants = context.platform.GetPlatform() == kBuildPlatformIOS ? ToView(ios_variants) : ToView(macos_variants);
		for (auto & config : context.platform.GetTargetConfigurations())
		{
			auto output_directory = config->GetPath(kOutputDirectory);
			if (!output_directory.path) continue;
			auto directory = File::CorrectTrailingStroke(TranslateVariables(ResolvePath(context.project.GetRoot(), output_directory)));
			auto bundle = IsBundle(*config);
			auto product_is_directory = config->GetProductType() == kOutputType_app || bundle;
			auto filename = ToWString(ProductFilename(*config, bundle));
			for (auto variant : variants) for (auto architecture : config->GetArchitectures())
			{
				auto resolved_directory = Replace(directory, L"$(CURRENT_ARCH)", ToWString(ArchitectureName(architecture)));
				resolved_directory = Replace(resolved_directory, L"$(PLATFORM_NAME)", ToWString(variant));
				auto resolved_filename = Replace(filename, L"$(CURRENT_ARCH)", ToWString(ArchitectureName(architecture)));
				resolved_filename = Replace(resolved_filename, L"$(PLATFORM_NAME)", ToWString(variant));
				if (product_is_directory) outputs.Set(Join(resolved_directory, resolved_filename, File::kStroke));
				else outputs.Set(Join(resolved_directory, resolved_filename));
				outputs.Set(Join(resolved_directory, resolved_filename, L".dSYM", File::kStroke));
			}
		}
	}
}

void ValidateTarget(const Context & context)
{
	auto configurations = context.platform.GetTargetConfigurations();
	Require(True(configurations), "target", "has no configurations");
	auto & first = configurations[0];
	auto first_bundle = IsBundle(first);
	GetConfigurationInvariant(context.platform, kSources,
		[](const TargetConfiguration & configuration) { return FlattenPaths(*GetFiles(configuration).sources); },
		[](ArrayView<PathDesc> a, ArrayView<PathDesc> b) { return ComparePaths(a, b); });
	GetConfigurationInvariant(context.platform, kOtherFiles,
		[](const TargetConfiguration & configuration) { return FlattenPaths(*GetFiles(configuration).other); },
		[](ArrayView<PathDesc> a, ArrayView<PathDesc> b) { return ComparePaths(a, b); });
	auto first_product = ProductFilename(first, first_bundle);
	auto platform_index = PlatformIndex(context.platform.GetPlatform());
	auto first_bundle_identifier = GetConfigurationInvariant(context.platform, kXcodeBundleIdentifierProperties[platform_index],
		[platform_index](const TargetConfiguration & configuration) { return configuration.GetString(kXcodeBundleIdentifierProperties[platform_index]); });
	GetConfigurationInvariant(context.platform, kXcodeInfoPlistProperties[platform_index],
		[platform_index](const TargetConfiguration & configuration) { return configuration.GetPath(kXcodeInfoPlistProperties[platform_index]); },
		[](const PathDesc & a, const PathDesc & b) { return ComparePath(a, b); });
	GetConfigurationInvariant(context.platform, kXcodeEntitlementsProperties[platform_index],
		[platform_index](const TargetConfiguration & configuration) { return configuration.GetPath(kXcodeEntitlementsProperties[platform_index]); },
		[](const PathDesc & a, const PathDesc & b) { return ComparePath(a, b); });
	GetConfigurationInvariant(context.platform, kXcodeIconProperties[platform_index],
		[platform_index](const TargetConfiguration & configuration) { return configuration.GetPath(kXcodeIconProperties[platform_index]); },
		[](const PathDesc & a, const PathDesc & b) { return ComparePath(a, b); });
	if (context.platform.GetPlatform() == kBuildPlatformIOS)
	{
		GetConfigurationInvariant(context.platform, kXcodeLaunchScreenProperty,
			[](const TargetConfiguration & configuration) { return configuration.GetPath(kXcodeLaunchScreenProperty); },
			[](const PathDesc & a, const PathDesc & b) { return ComparePath(a, b); });
	}
	for (auto & config : configurations)
	{
		auto bundle = IsBundle(config);
		ProductType(config, context.platform.GetPlatform(), bundle);
		Require(bundle == first_bundle && ProductFilename(config, bundle) == first_product, "target", "product identity must be identical across configurations");
		Require(config->GetNumber(kXcodeMinimumVersionProperties[platform_index]) >= 0, kXcodeMinimumVersionProperties[platform_index], "expected a non-negative number");
		auto output_type = config->GetProductType();
		auto automatically_signable = output_type == kOutputType_app || bundle;
		auto codesign = config->GetBool(kXcodeCodesignProperties[platform_index], true);
		auto development_team = config->GetString(kXcodeDevelopmentTeamProperties[platform_index]);
		auto entitlements = config->GetPath(kXcodeEntitlementsProperties[platform_index]);
		Require(!entitlements.path || output_type == kOutputType_app || bundle, "entitlements", "require an app or bundled product");
		Require(!development_team || development_team.GetSize() == 10, kXcodeDevelopmentTeamProperties[platform_index], "expected a 10-character team identifier");
		Require(!automatically_signable || !codesign || !development_team || True(first_bundle_identifier), "codesign", "automatic signing requires a bundle identifier");
		if (context.platform.GetPlatform() == kBuildPlatformIOS && output_type == kOutputType_app) Require(True(first_bundle_identifier), kXcodeBundleIdentifierProperties[1], "is required for app targets");
		if (context.platform.GetPlatform() == kBuildPlatformIOS)
		{
			auto launch_screen = config->GetPath(kXcodeLaunchScreenProperty);
			if (launch_screen.path)
			{
				Require(config->GetProductType() == kOutputType_app, kXcodeLaunchScreenProperty, "is only valid for app targets");
				Require(MakeKey32(Lowercase(File::SplitExtension(launch_screen.path).b)) == K32("storyboard"), kXcodeLaunchScreenProperty, "must be a storyboard");
			}
		}
		Require(!bundle || output_type == kOutputType_app || output_type == kOutputType_dynamic_library, kXcodeBundleIdentifierProperties[0], "is only valid for app or dynamic_library products");
		for (auto architecture : config->GetArchitectures()) ArchitectureName(architecture);
	}
}

WString XcodeString(CString::View value)
{
	return ToWString(value);
}

WString XcodeString(WString::View value)
{
	return value;
}

template <class VALUE> void WriteList(Data::Archive & output, UInt indent, CString::View name, const Array <VALUE> & values)
{
	if (values)
	{
		WriteLine(output, indent, Join(EscapePbx(name), " = ("));
		for (auto & value : values) WriteLine(output, indent + 1, Join(EscapePbx(TranslateVariables(XcodeString(value))), L','));
		WriteLine(output, indent, Join(");"));
	}
}

template <class VALUE> void WriteSetting(Data::Archive & output, CString::View name, VALUE value)
{
	if (value) WriteLine(output, 4, Join(ToWString(EscapePbx(name)), L" = ", EscapePbx(TranslateVariables(XcodeString(value))), L';'));
}

void WriteSetting(Data::Archive & output, CString::View name, Float32 value)
{
	if (value) WriteSetting(output, name, ToCString(value, 2, true));
}

Array <WString> PathValues(ArrayView <PathDesc> paths)
{
	Array<WString> result;
	for (auto & path : paths) result.Push(RootPath(path));
	if (result) result.Push(L"$(inherited)");
	return result;
}

Array <CString> Definitions(const TargetConfiguration & config)
{
	Array<CString> result = config.GetDefinitions();
	if (result) result.Push("$(inherited)");
	return result;
}

Array<CString> WarningFlags(const TargetConfiguration & config)
{
	Array<CString> result = config.GetStrings(kCompilerOptions);
	auto warning_level = config.GetEnum<WarningLevel>(kWarningLevel, kWarningLevelNames, kWarningLevel_standard);
	for (auto option : GnuWarningOptions(config)) result.Push(option);
	result.Push(warning_level == kWarningLevel_pedantic ? "-Wnullability-completeness" : "-Wno-nullability-completeness");
	for (auto option : ClangFloatingPointOptions(config)) result.Push(option);
	return result;
}

Array <WString> LinkFlags(const Context & context, const TargetConfiguration & config)
{
	Array <WString> result;
	for (auto dependency : context.link_dependencies)
	{
		if (dependency->IsLibrary()) result.Push(DependencyLibraryPath(context, *dependency, config.GetName()));
	}
	auto platform_index = PlatformIndex(context.platform.GetPlatform());
	for (auto & framework : config.GetAppendableStrings(kXcodeFrameworkProperties[platform_index]))
	{
		result.Push(L"-framework");
		result.Push(ToWString(framework));
	}
	return result;
}

void WriteBuildSettings(Data::Archive & output, const Context & context, const TargetConfiguration & config)
{
	constexpr CString::View standards[] = { "c++17", "c++20" };
	constexpr CString::View optimizations[] = { "0", "s", "2", "3" };

	WriteSetting(output, "PROJECT_ROOT", "$(PROJECT_DIR)/../..");
	WriteSetting(output, "SDKROOT", context.platform.GetPlatform() == kBuildPlatformIOS ? "iphoneos" : "macosx");
	auto platform_index = PlatformIndex(context.platform.GetPlatform());
	if (context.platform.GetPlatform() == kBuildPlatformIOS) WriteSetting(output, "IPHONEOS_DEPLOYMENT_TARGET", config.GetNumber(kXcodeMinimumVersionProperties[platform_index]));
	else WriteSetting(output, "MACOSX_DEPLOYMENT_TARGET", config.GetNumber(kXcodeMinimumVersionProperties[platform_index]));
	WriteSetting(output, "ALWAYS_SEARCH_USER_PATHS", "NO");
	if (context.platform.GetPlatform() == kBuildPlatformIOS)
	{
		WriteSetting(output, "SUPPORTED_PLATFORMS", "iphoneos iphonesimulator");
		WriteList(output, 4, "ARCHS[sdk=iphoneos*]", Architectures(config));
		WriteList(output, 4, "ARCHS[sdk=iphonesimulator*]", Architectures(config));
	}
	else
	{
		WriteList(output, 4, "ARCHS", Architectures(config));
		WriteSetting(output, "ONLY_ACTIVE_ARCH", config.GetBool(kMacOSBuildAllArchitectures) ? "NO" : "YES");
	}
	auto output_type = config.GetProductType();
	auto output_extension = config.GetString(kOutputExtension);
	auto optimization = config.GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none);
	auto optimized = optimization != kOptimization_none;
	auto debug_information = config.GetBool(kDebugInformation, !optimized);
	auto dead_strip = config.GetBool(kDeadStrip, optimized);
	WriteSetting(output, "PRODUCT_NAME", config.GetString(kOutputName, config.platform->target->project->GetName()));
	WriteSetting(output, "CLANG_CXX_LANGUAGE_STANDARD", standards[config.GetEnum<CppStandard>(kCppStandard, kCppStandardNames, kCppStandard_cxx20)]);
	WriteSetting(output, "CLANG_ENABLE_OBJC_ARC", config.GetBool(kXcodeArcProperties[platform_index]) ? "YES" : "NO");
	WriteSetting(output, "GCC_ENABLE_CPP_RTTI", config.GetBool(kRtti, true) ? "YES" : "NO");
	WriteSetting(output, "GCC_OPTIMIZATION_LEVEL", optimizations[optimization]);
	WriteList(output, 4, "GCC_PREPROCESSOR_DEFINITIONS", Definitions(config));
	WriteList(output, 4, "OTHER_CPLUSPLUSFLAGS", WarningFlags(config));
	WriteList(output, 4, "HEADER_SEARCH_PATHS", PathValues(FlattenPaths(config.GetPaths(true, kIncludeDirectories))));
	WriteList(output, 4, "OTHER_LDFLAGS", LinkFlags(context, config));
	if (context.platform.GetPlatform() == kBuildPlatformMacOS)
		WriteSetting(output, "EXPORTED_SYMBOLS_FILE", RootPath(config.GetPath(kMacOSExportedSymbols)));
	WriteSetting(output, "GCC_GENERATE_DEBUGGING_SYMBOLS", debug_information ? "YES" : "NO");
	if (debug_information) WriteSetting(output, "DEBUG_INFORMATION_FORMAT", "dwarf-with-dsym");
	WriteSetting(output, "DEAD_CODE_STRIPPING", dead_strip ? "YES" : "NO");
	WriteSetting(output, "CONFIGURATION_BUILD_DIR", RootPath(config.GetPath(kOutputDirectory)));
	WriteSetting(output, "CONFIGURATION_TEMP_DIR", "$(PROJECT_DIR)/intermediate/$(CONFIGURATION)/$(PLATFORM_NAME)/$(TARGET_NAME)");
	auto bundle = IsBundle(config);
	auto extension = ProductExtension(config, bundle);
	if (output_type == kOutputType_dynamic_library && bundle)
	{
		WriteSetting(output, "MACH_O_TYPE", "mh_dylib");
		WriteSetting(output, "WRAPPER_EXTENSION", extension);
	}
	else if (output_extension && output_type == kOutputType_app)
	{
		WriteSetting(output, "WRAPPER_EXTENSION", extension);
	}
	else if (output_extension && output_type != kOutputType_app)
	{
		WriteSetting(output, "EXECUTABLE_EXTENSION", extension);
	}
	WriteSetting(output, "PRODUCT_BUNDLE_IDENTIFIER", config.GetString(kXcodeBundleIdentifierProperties[platform_index]));
	auto info_plist = config.GetPath(kXcodeInfoPlistProperties[platform_index]);
	auto launch_screen = context.platform.GetPlatform() == kBuildPlatformIOS ? config.GetPath(kXcodeLaunchScreenProperty) : PathDesc {};
	if (info_plist.path)
	{
		WriteSetting(output, "GENERATE_INFOPLIST_FILE", launch_screen.path ? "YES" : "NO");
		WriteSetting(output, "INFOPLIST_FILE", RootPath(info_plist));
	}
	else if (output_type == kOutputType_app || bundle)
	{
		WriteSetting(output, "GENERATE_INFOPLIST_FILE", "YES");
	}
	WriteSetting(output, "CODE_SIGN_ENTITLEMENTS", RootPath(config.GetPath(kXcodeEntitlementsProperties[platform_index])));
	if (config.GetPath(kXcodeIconProperties[platform_index]).path) WriteSetting(output, "ASSETCATALOG_COMPILER_APPICON_NAME", "AppIcon");
	if (launch_screen.path)
	{
		auto filename = File::SplitFilename(launch_screen.path).b;
		WriteSetting(output, "INFOPLIST_KEY_UILaunchStoryboardName", EncodeUTF8(File::SplitExtension(filename).a));
	}
	auto automatically_signable = output_type == kOutputType_app || bundle;
	auto locally_signable = context.platform.GetPlatform() == kBuildPlatformMacOS && (automatically_signable || output_type == kOutputType_console);
	if (automatically_signable || locally_signable)
	{
		if (config.GetBool(kXcodeCodesignProperties[platform_index], true))
		{
			if (auto development_team = config.GetString(kXcodeDevelopmentTeamProperties[platform_index]); automatically_signable && development_team)
			{
				WriteSetting(output, "CODE_SIGN_STYLE", "Automatic");
				WriteSetting(output, "DEVELOPMENT_TEAM", development_team);
			}
			else if (locally_signable)
			{
				WriteSetting(output, "CODE_SIGN_IDENTITY", "-");
				WriteSetting(output, "CODE_SIGN_STYLE", "Manual");
			}
		}
		else
		{
			WriteSetting(output, "CODE_SIGNING_ALLOWED", "NO");
		}
	}
	WriteSetting(output, "ENABLE_USER_SCRIPT_SANDBOXING", "NO");
}

Array<Target *> ProjectDependencies(const Context & context)
{
	Array<Target *> result;
	for (auto dependency : context.dependencies) if (!dependency->IsLibrary()) result.Push(dependency);
	for (auto dependency : context.link_dependencies) if (!dependency->IsLibrary() && !Search(result, dependency)) result.Push(dependency);
	return result;
}

CString DependencyProductFilename(const Context & context, const Target & dependency)
{
	auto platform = dependency.FindPlatform(context.platform.GetPlatform());
	REFLEX_ASSERT(platform && platform->GetTargetConfigurations());
	auto & config = platform->GetTargetConfigurations()[0];
	return ProductFilename(config, IsBundle(config));
}

CString::View DependencyProductFileType(const Context & context, const Target & dependency)
{
	auto platform = dependency.FindPlatform(context.platform.GetPlatform());
	REFLEX_ASSERT(platform && platform->GetTargetConfigurations());
	auto & config = platform->GetTargetConfigurations()[0];
	constexpr CString::View types[] = { "compiled.mach-o.executable", "wrapper.application", "compiled.mach-o.dylib", "archive.ar" };
	return types[config->GetProductType()];
}

void WriteFileReferences(Data::Archive & output, const Context & context, const Array<PathDesc> & sources, const Array<PathDesc> & headers, const Array<PathDesc> & other, const Array<PathDesc> & resources, const Array<CString> & frameworks, bool bundle)
{
	PbxScope scope(output, "PBXFileReference");
	Array<PathDesc> written_paths;
	for (auto kind : { UInt(0), UInt(1), UInt(2), UInt(3) })
	{
		auto & paths = kind == 0 ? sources : kind == 1 ? headers : kind == 2 ? other : resources;
		for (auto & path : paths)
		{
			if (ContainsPath(written_paths, path)) continue;
			written_paths.Push(path);
			auto name = Filename(path);
			auto id = GenerateID(context.target.GetName(), { "file" }, EncodeUTF8(path.path));
			WriteLine(output, 2, Join(id, " /* ", name, " */ = {isa = PBXFileReference; lastKnownFileType = ", FileType(path), "; name = ", EscapePbx(name), "; path = ", EncodeUTF8(EscapePbx(ProjectPath(context, path))), "; sourceTree = ", EscapePbx(path.is_absolute ? "<absolute>" : "SOURCE_ROOT"), "; };"));
		}
	}
	for (auto & framework : frameworks)
	{
		auto id = GenerateID(context.target.GetName(), { "framework" }, framework);
		WriteLine(output, 2, Join(id, " /* ", framework, ".framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = ", EscapePbx(Join(framework, ".framework")), "; path = ", EscapePbx(Join("System/Library/Frameworks/", framework, ".framework")), "; sourceTree = SDKROOT; };"));
	}
	for (auto dependency : ProjectDependencies(context))
	{
		auto project_directory = Join(GetProjectFolder(*dependency->project, context.platform.GetPlatform()), ToWString(dependency->GetName()), L".xcodeproj", File::kStroke);
		auto project_path = File::MakeRelativePath(context.directory, project_directory);
		auto project_id = MakeDependencyID(context, *dependency, "project");
		auto reference_id = MakeDependencyID(context, *dependency, "product-reference");
		auto product = DependencyProductFilename(context, *dependency);
		WriteLine(output, 2, Join(project_id, " /* ", dependency->GetName(), ".xcodeproj */ = {isa = PBXFileReference; lastKnownFileType = \"wrapper.pb-project\"; name = ", EscapePbx(Join(dependency->GetName(), ".xcodeproj")), "; path = ", EscapePbx(EncodeUTF8(project_path)), "; sourceTree = SOURCE_ROOT; };"));
		WriteLine(output, 2, Join(reference_id, " /* ", product, " */ = {isa = PBXReferenceProxy; fileType = ", DependencyProductFileType(context, *dependency), "; path = ", EscapePbx(product), "; remoteRef = ", MakeDependencyID(context, *dependency, "product-proxy"), " /* PBXContainerItemProxy */; sourceTree = BUILT_PRODUCTS_DIR; };"));
	}
	const CString::View kProductFileTypes[] = { "compiled.mach-o.executable", "wrapper.application", bundle ? "wrapper.cfbundle" : "compiled.mach-o.dylib", "archive.ar" };
	auto & config = context.platform.GetTargetConfigurations()[0];
	WriteLine(output, 2, Join(MakeID(context, "product"), " /* ", ProductFilename(config, bundle), " */ = {isa = PBXFileReference; explicitFileType = ", kProductFileTypes[config->GetProductType()], "; includeInIndex = 0; path = ", EscapePbx(ProductFilename(config, bundle)), "; sourceTree = BUILT_PRODUCTS_DIR; };"));
}

void WriteBuildFiles(Data::Archive & output, const Context & context, const Array<PathDesc> & sources, const Array<PathDesc> & resources)
{
	PbxScope scope(output, "PBXBuildFile");
	for (auto & path : sources)
	{
		auto name = Filename(path);
		WriteLine(output, 2, Join(GenerateID(context.target.GetName(), { "source-build" }, EncodeUTF8(path.path)), " /* ", name, " in Sources */ = {isa = PBXBuildFile; fileRef = ", GenerateID(context.target.GetName(), { "file" }, EncodeUTF8(path.path)), " /* ", name, " */; };"));
	}
	for (auto & path : resources)
	{
		auto name = Filename(path);
		WriteLine(output, 2, Join(GenerateID(context.target.GetName(), { "resource-build" }, EncodeUTF8(path.path)), " /* ", name, " in Resources */ = {isa = PBXBuildFile; fileRef = ", GenerateID(context.target.GetName(), { "file" }, EncodeUTF8(path.path)), " /* ", name, " */; };"));
	}
	for (auto dependency : context.link_dependencies)
	{
		if (dependency->IsLibrary()) continue;
		auto product = DependencyProductFilename(context, *dependency);
		WriteLine(output, 2, Join(MakeDependencyID(context, *dependency, "product-build"), " /* ", product, " in Frameworks */ = {isa = PBXBuildFile; fileRef = ", MakeDependencyID(context, *dependency, "product-reference"), " /* ", product, " */; };"));
	}
}

void WriteGroup(Data::Archive & output, CString::View id, CString::View name, const Array<CString> & children)
{
	PbxObject object(output, id, name);
	object.WriteSetting("isa", "PBXGroup");
	WriteLine(output, 3, "children = (");
	for (auto & child : children) WriteLine(output, 4, Join(child, ","));
	WriteLine(output, 3, ");");
	if (name) object.WriteSetting("name", EscapePbx(name));
	object.WriteSetting("sourceTree", "\"<group>\"");
}

void WriteGroups(Data::Archive & output, const Context & context, const PathGroup & groups, const Array<PathDesc> & resources, const Array<CString> & frameworks, bool bundle)
{
	REFLEX_LOCAL(void,WriteTree)(Data::Archive & output, const Context & context, const PathGroup & group, CString path, CString id, CString name, Array<CString> children, Array<PathDesc> & written_paths)
	{
		for (auto & item : group.paths)
		{
			auto & file = item.a;
			if (ContainsPath(written_paths, file)) continue;
			written_paths.Push(file);
			children.Push(Join(GenerateID(context.target.GetName(), { "file" }, EncodeUTF8(file.path)), " /* ", Filename(file), " */"));
		}
		for (auto & child : group)
		{
			auto child_path = path ? Join(path, "/", child.name) : child.name;
			auto child_id = GenerateID(context.target.GetName(), { "group" }, child_path);
			children.Push(Join(child_id, " /* ", child.name, " */"));
			Call(output, context, child, std::move(child_path), child_id, child.name, {}, written_paths);
		}
		WriteGroup(output, id, name, children);
	};
	REFLEX_END

	Array<CString> main_children =
	{
		Join(MakeID(context, "frameworks-group"), " /* Frameworks */"),
		Join(MakeID(context, "products-group"), " /* Products */")
	};
	if (ProjectDependencies(context)) main_children.Push(Join(MakeID(context, "projects-group"), " /* Dependencies */"));
	auto grouped_paths = FlattenPaths(groups);
	for (auto & resource : resources)
	{
		if (ContainsPath(grouped_paths, resource)) continue;
		grouped_paths.Push(resource);
		main_children.Push(Join(GenerateID(context.target.GetName(), { "file" }, EncodeUTF8(resource.path)), " /* ", Filename(resource), " */"));
	}
	Array<CString> product_children = { Join(MakeID(context, "product"), " /* ", ProductFilename(context.platform.GetTargetConfigurations()[0], bundle), " */") };
	Array<CString> framework_children;
	for (auto & framework : frameworks) framework_children.Push(Join(GenerateID(context.target.GetName(), { "framework" }, framework), " /* ", framework, ".framework */"));
	Array<Target *> dependencies = ProjectDependencies(context);
	Array<CString> project_children;
	for (auto dependency : dependencies) project_children.Push(Join(MakeDependencyID(context, *dependency, "project"), " /* ", dependency->GetName(), ".xcodeproj */"));
	Array<PathDesc> written_paths;

	PbxScope scope(output, "PBXGroup");
	WriteTree::Call(output, context, groups, {}, MakeID(context, "main-group"), {}, std::move(main_children), written_paths);
	WriteGroup(output, MakeID(context, "products-group"), "Products", product_children);
	WriteGroup(output, MakeID(context, "frameworks-group"), "Frameworks", framework_children);
	if (project_children) WriteGroup(output, MakeID(context, "projects-group"), "Dependencies", project_children);
	for (auto dependency : dependencies)
	{
		auto product = DependencyProductFilename(context, *dependency);
		Array<CString> product_children = { Join(MakeDependencyID(context, *dependency, "product-reference"), " /* ", product, " */") };
		WriteGroup(output, MakeDependencyID(context, *dependency, "products-group"), dependency->GetName(), product_children);
	}
}

void WritePhase(Data::Archive & output, CString::View id, CString::View isa, CString::View name, ArrayView <CString> files)
{
	PbxScope scope(output, isa);
	PbxObject object(output, id, name);
	object.WriteSetting("isa", isa);
	object.WriteSetting("buildActionMask", "2147483647");
	WriteLine(output, 3, "files = (");
	for (auto & file : files) WriteLine(output, 4, Join(file, ","));
	WriteLine(output, 3, ");");
	object.WriteSetting("runOnlyForDeploymentPostprocessing", "0");
}

Array <CString> BuildFileIds(const Context & context, const Array<PathDesc> & paths, CString::View kind, CString::View phase)
{
	Array<CString> result;
	for (auto & path : paths) result.Push(Join(GenerateID(context.target.GetName(), { Join(kind, "-build") }, EncodeUTF8(path.path)), " /* ", Filename(path), " in ", phase, " */"));
	return result;
}

void WriteBuildPhases(Data::Archive & output, const Context & context, const Array<PathDesc> & sources, const Array<PathDesc> & resources)
{
	WritePhase(output, MakeID(context, "sources"), "PBXSourcesBuildPhase", "Sources", BuildFileIds(context, sources, "source", "Sources"));
	WritePhase(output, MakeID(context, "resources"), "PBXResourcesBuildPhase", "Resources", BuildFileIds(context, resources, "resource", "Resources"));
	Array<CString> dependencies;
	for (auto dependency : context.link_dependencies)
	{
		if (dependency->IsLibrary()) continue;
		auto product = DependencyProductFilename(context, *dependency);
		dependencies.Push(Join(MakeDependencyID(context, *dependency, "product-build"), " /* ", product, " in Frameworks */"));
	}
	WritePhase(output, MakeID(context, "frameworks"), "PBXFrameworksBuildPhase", "Frameworks", dependencies);
}

System::Platform ToNativePlatform(const Context & context)
{
	switch (context.platform.GetPlatform())
	{
	case kBuildPlatformMacOS:
		return System::kPlatformMacOS;

	case kBuildPlatformIOS:
		return System::kPlatformIOS;

	default:
		ThrowError(ToView("invalid platform"), kBuildPlatforms[context.platform.GetPlatform()]);
		return System::kNumPlatform;
	}
}

bool HasBuildActions(const Context & context, BuildPhase property)
{
	for (auto & config : context.platform.GetTargetConfigurations())
	{
		if (config->GetBuildActions(property, ToNativePlatform(context))) return true;
	}
	return false;
}

void WriteShellPhases(Data::Archive & output, const Context & context, BuildPhase phase, Array<CString> & build_phases)
{
	constexpr WString::View kEscapedLineBreak = L"\\n";

	auto native_platform = ToNativePlatform(context);

	UInt action_count = 0;
	for (auto & config : context.platform.GetTargetConfigurations())
	{
		auto actions = config->GetBuildActions(phase, native_platform);
		action_count = Max(action_count, actions.GetSize());
	}
	for (UInt action_index = 0; action_index < action_count; ++action_index)
	{
		bool configuration_specific = context.platform.GetTargetConfigurations().size > 1;
		if (configuration_specific)
		{
			BuildActionDesc common_action;
			bool has_common_action = false;
			for (auto & config : context.platform.GetTargetConfigurations())
			{
				auto actions = config->GetBuildActions(phase, native_platform);
				if (action_index >= actions.GetSize()) { has_common_action = false; break; }
				auto & action = actions[action_index];
				if (has_common_action)
				{
					if (!CompareBuildActions(common_action, action))
					{
						has_common_action = false;
						break;
					}
				}
				else
				{
					common_action = action;
					has_common_action = true;
				}
			}
			configuration_specific = !has_common_action;
		}
		BuildActionDesc first_action;
		bool has_action = false;
		bool always_out_of_date = configuration_specific;
		Array <WString> inputs, outputs;
		WString command = L"\"";
		auto append_line = [&](WString::View line)
		{
			command.Append(EscapeQuotedString(line));
			command.Append(kEscapedLineBreak);
		};
		if (configuration_specific) append_line(L"case \"$CONFIGURATION\" in");
		for (auto & config : context.platform.GetTargetConfigurations())
		{
			auto actions = config->GetBuildActions(phase, native_platform);
			if (action_index >= actions.GetSize()) continue;
			auto & action = actions[action_index];
			if (!configuration_specific && has_action) continue;
			if (!has_action) { first_action = action; has_action = true; }
			always_out_of_date = always_out_of_date || action.always_run || action.outputs.Empty();
			if (!configuration_specific)
			{
				for (auto & path : action.inputs) inputs.Push(RootPath(path));
			}
			for (auto & path : action.outputs)
			{
				auto value = RootPath(path);
				if (!Search(outputs, value)) outputs.Push(std::move(value));
			}
			if (configuration_specific)
			{
				append_line(Join(L"\t", ShellQuote(ToWString(config->GetName())), L")"));
				append_line(Join(L"\t\t", ShellCommand(action)));
				append_line(L"\t\t;;");
			}
			else append_line(ShellCommand(action));
		}
		if (!has_action) continue;
		if (configuration_specific) append_line(L"esac");
		command.Push(L'"');
		auto & action = first_action;
		auto id = GenerateID(context.target.GetName(), { "script", kBuildPhases[phase] }, ToCString(action_index));
		build_phases.Push(Join(id, " /* ", action.name, " */"));
		PbxObject object(output, id, action.name);
		object.WriteSetting("isa", "PBXShellScriptBuildPhase");
		if (always_out_of_date) object.WriteSetting("alwaysOutOfDate", "1");
		object.WriteSetting("buildActionMask", "2147483647");
		WriteList(output, 3, "inputPaths", inputs);
		if (action.name) object.WriteSetting("name", EscapePbx(action.name));
		WriteList(output, 3, "outputPaths", outputs);
		object.WriteSetting("runOnlyForDeploymentPostprocessing", "0");
		object.WriteSetting("shellPath", "/bin/sh");
		object.WriteSetting("shellScript", command);
		object.WriteSetting("showEnvVarsInLog", "0");
	}
}

void WriteConfigurations(Data::Archive & output, const Context & context)
{
	{
		PbxScope scope(output, "XCBuildConfiguration");
		for (auto & config : context.platform.GetTargetConfigurations())
		{
			{
				PbxObject object(output, GenerateID(context.target.GetName(), { "project-configuration" }, config->GetName()), config->GetName());
				object.WriteSetting("isa", "XCBuildConfiguration");
				object.WriteSetting("buildSettings", "{}");
				object.WriteSetting("name", EscapePbx(config->GetName()));
			}
			{
				PbxObject object(output, GenerateID(context.target.GetName(), { "configuration" }, config->GetName()), config->GetName());
				object.WriteSetting("isa", "XCBuildConfiguration");
				WriteLine(output, 3, "buildSettings = {");
				WriteBuildSettings(output, context, config);
				WriteLine(output, 3, "};");
				object.WriteSetting("name", EscapePbx(config->GetName()));
			}
		}
	}

	{
		PbxScope scope(output, "XCConfigurationList");
		{
			PbxObject object(output, MakeID(context, "configurations"), Join("Build configuration list for PBXNativeTarget ", EscapePbx(context.target.GetName())));
			object.WriteSetting("isa", "XCConfigurationList");
			WriteLine(output, 3, "buildConfigurations = (");
			for (auto & config : context.platform.GetTargetConfigurations())
			{
				WriteLine(output, 4, Join(GenerateID(context.target.GetName(), { "configuration" }, config->GetName()), " /* ", config->GetName(), " */,"));
			}
			WriteLine(output, 3, ");");
			object.WriteSetting("defaultConfigurationIsVisible", "0");
			object.WriteSetting("defaultConfigurationName", EscapePbx(context.project.GetDefaultConfiguration()));
		}
		{
			PbxObject object(output, MakeID(context, "project-configurations"), "Build configuration list for PBXProject");
			object.WriteSetting("isa", "XCConfigurationList");
			WriteLine(output, 3, "buildConfigurations = (");
			for (auto & config : context.platform.GetTargetConfigurations())
			{
				WriteLine(output, 4, Join(GenerateID(context.target.GetName(), { "project-configuration" }, config->GetName()), " /* ", config->GetName(), " */,"));
			}
			WriteLine(output, 3, ");");
			object.WriteSetting("defaultConfigurationIsVisible", "0");
			object.WriteSetting("defaultConfigurationName", EscapePbx(context.project.GetDefaultConfiguration()));
		}
	}
}

void WriteProject(const Context & context)
{
	auto & config = context.platform.GetTargetConfigurations()[0];
	auto bundle = IsBundle(config);
	auto sources = CollectPaths(context.platform, 0);
	auto headers = CollectPaths(context.platform, 1);
	auto other = CollectPaths(context.platform, 2);
	auto resources = CollectPaths(context.platform, 3);
	auto frameworks = CollectFrameworks(context.platform);
	auto groups = ConsolidatePathGroups(context.platform);
	auto grouped_paths = FlattenPaths(*groups);
	for (auto & resource : resources)
	{
		if (ContainsPath(grouped_paths, resource)) continue;
		AcquirePathGroup(*groups, kBuildPlatforms[context.platform.GetPlatform()])->paths.Push({ resource, Key32(kOtherFiles) });
		grouped_paths.Push(resource);
	}

	Data::Archive output;
	WriteLine(output, 0, "// !$*UTF8*$!");
	WriteLine(output, 0, "{");
	WriteLine(output, 1, "archiveVersion = 1;");
	WriteLine(output, 1, "classes = {};");
	WriteLine(output, 1, "objectVersion = 56;");
	WriteLine(output, 1, "objects = {");
	WriteLine(output, 0);
	WriteBuildFiles(output, context, sources, resources);
	WriteFileReferences(output, context, sources, headers, other, resources, frameworks, bundle);
	WriteGroups(output, context, *groups, resources, frameworks, bundle);
	WriteBuildPhases(output, context, sources, resources);
	if (auto dependencies = ProjectDependencies(context); dependencies)
	{
		PbxScope scope(output, "PBXContainerItemProxy");
		for (auto dependency : dependencies)
		{
			{
				PbxObject object(output, MakeDependencyID(context, *dependency, "target-proxy"), "PBXContainerItemProxy");
				object.WriteSetting("isa", "PBXContainerItemProxy");
				object.WriteSetting("containerPortal", Join(MakeDependencyID(context, *dependency, "project"), " /* ", dependency->GetName(), ".xcodeproj */"));
				object.WriteSetting("proxyType", "1");
				object.WriteSetting("remoteGlobalIDString", TargetID(*dependency, context.platform.GetPlatform()));
				object.WriteSetting("remoteInfo", EscapePbx(dependency->GetName()));
			}
			{
				PbxObject product_object(output, MakeDependencyID(context, *dependency, "product-proxy"), "PBXContainerItemProxy");
				product_object.WriteSetting("isa", "PBXContainerItemProxy");
				product_object.WriteSetting("containerPortal", Join(MakeDependencyID(context, *dependency, "project"), " /* ", dependency->GetName(), ".xcodeproj */"));
				product_object.WriteSetting("proxyType", "2");
				product_object.WriteSetting("remoteGlobalIDString", ProductID(*dependency, context.platform.GetPlatform()));
				product_object.WriteSetting("remoteInfo", EscapePbx(DependencyProductFilename(context, *dependency)));
			}
		}
	}
	if (context.dependencies)
	{
		PbxScope scope(output, "PBXTargetDependency");
		for (auto dependency : context.dependencies)
		{
			PbxObject object(output, MakeDependencyID(context, *dependency, "target-dependency"), dependency->GetName());
			object.WriteSetting("isa", "PBXTargetDependency");
			object.WriteSetting("targetProxy", Join(MakeDependencyID(context, *dependency, "target-proxy"), " /* PBXContainerItemProxy */"));
		}
	}
	Array<CString> pre_build, post_build;
	if (HasBuildActions(context, kBuildPhasePreBuild) || HasBuildActions(context, kBuildPhasePostBuild))
	{
		PbxScope scope(output, "PBXShellScriptBuildPhase");
		WriteShellPhases(output, context, kBuildPhasePreBuild, pre_build);
		WriteShellPhases(output, context, kBuildPhasePostBuild, post_build);
	}

	{
		PbxScope scope(output, "PBXNativeTarget");
		PbxObject object(output, MakeID(context, "target"), context.target.GetName());
		object.WriteSetting("isa", "PBXNativeTarget");
		object.WriteSetting("buildConfigurationList", Join(MakeID(context, "configurations"), " /* Build configuration list */"));
		WriteLine(output, 3, "buildPhases = (");
		for (auto & phase : pre_build) WriteLine(output, 4, Join(phase, ","));
		WriteLine(output, 4, Join(MakeID(context, "sources"), " /* Sources */,"));
		WriteLine(output, 4, Join(MakeID(context, "frameworks"), " /* Frameworks */,"));
		WriteLine(output, 4, Join(MakeID(context, "resources"), " /* Resources */,"));
		for (auto & phase : post_build) WriteLine(output, 4, Join(phase, ","));
		WriteLine(output, 3, ");");
		object.WriteSetting("buildRules", "()");
		if (context.dependencies)
		{
			WriteLine(output, 3, "dependencies = (");
			for (auto dependency : context.dependencies) WriteLine(output, 4, Join(MakeDependencyID(context, *dependency, "target-dependency"), " /* ", dependency->GetName(), " */,"));
			WriteLine(output, 3, ");");
		}
		else object.WriteSetting("dependencies", "()");
		object.WriteSetting("name", EscapePbx(context.target.GetName()));
		object.WriteSetting("productName", "\"$(PRODUCT_NAME)\"");
		object.WriteSetting("productReference", Join(MakeID(context, "product"), " /* Product */"));
		object.WriteSetting("productType", EscapePbx(ProductType(config, context.platform.GetPlatform(), bundle)));
	}

	{
		PbxScope scope(output, "PBXProject");
		PbxObject object(output, MakeID(context, "project"), "Project object");
		object.WriteSetting("isa", "PBXProject");
		object.WriteSetting("attributes", "{ BuildIndependentTargetsInParallel = 1; LastUpgradeCheck = 2640; }");
		object.WriteSetting("buildConfigurationList", Join(MakeID(context, "project-configurations"), " /* Build configuration list */"));
		object.WriteSetting("compatibilityVersion", "\"Xcode 14.0\"");
		object.WriteSetting("developmentRegion", "en");
		object.WriteSetting("mainGroup", MakeID(context, "main-group"));
		object.WriteSetting("productRefGroup", Join(MakeID(context, "products-group"), " /* Products */"));
		object.WriteSetting("projectDirPath", "\"\"");
		object.WriteSetting("projectRoot", "\"\"");
		if (auto dependencies = ProjectDependencies(context); dependencies)
		{
			WriteLine(output, 3, "projectReferences = (");
			for (auto dependency : dependencies)
			{
				WriteLine(output, 4, "{");
				WriteLine(output, 5, Join("ProductGroup = ", MakeDependencyID(context, *dependency, "products-group"), " /* ", dependency->GetName(), " */;"));
				WriteLine(output, 5, Join("ProjectRef = ", MakeDependencyID(context, *dependency, "project"), " /* ", dependency->GetName(), ".xcodeproj */;"));
				WriteLine(output, 4, "},");
			}
			WriteLine(output, 3, ");");
		}
		object.WriteSetting("targets", Join("(", MakeID(context, "target"), " /* ", context.target.GetName(), " */,)"));
	}
	WriteConfigurations(output, context);
	WriteLine(output, 1, "};");
	WriteLine(output, 1, Join("rootObject = ", MakeID(context, "project"), " /* Project object */;"));
	WriteLine(output, 0, "}");

	auto project_directory = Join(context.directory, ToWString(context.target.GetName()), L".xcodeproj", File::kStroke);
	SaveFile(Join(project_directory, L"project.pbxproj"), output, context.platform.GetPlatform());
}

void WriteBuildableReference(XmlWriter & xml, const Context & context, CString::View reference)
{
	xml.EmptyElement("BuildableReference",
	{
		{ "BuildableIdentifier", "primary" },
		{ "BlueprintIdentifier", MakeID(context, "target") },
		{ "BuildableName", ProductFilename(context.platform.GetTargetConfigurations()[0], IsBundle(context.platform.GetTargetConfigurations()[0])) },
		{ "BlueprintName", context.target.GetName() },
		{ "ReferencedContainer", reference },
	});
}

void WriteBuildActionEntry(XmlWriter & xml, const Context & context, CString::View reference)
{
	XmlScope entry(xml, "BuildActionEntry",
	{
		{ "buildForTesting", "YES" },
		{ "buildForRunning", "YES" },
		{ "buildForProfiling", "YES" },
		{ "buildForArchiving", "YES" },
		{ "buildForAnalyzing", "YES" },
	});
	WriteBuildableReference(xml, context, reference);
}

void WriteLaunchEnvironment(XmlWriter & xml)
{
	XmlScope variables(xml, "EnvironmentVariables");
	xml.EmptyElement("EnvironmentVariable",
	{
		{ "key", "MALLOC_PERMIT_INSANE_REQUESTS" },
		{ "value", "1" },
		{ "isEnabled", "YES" },
	});
}

void WriteScheme(const Context & context)
{
	auto project_name = Join(context.target.GetName(), ".xcodeproj");
	auto reference = Join("container:", project_name);
	auto [debug,release] = FindDebugAndReleaseConfigurations(context.platform);
	auto debug_name = debug ? debug->GetName() : context.project.GetDefaultConfiguration();
	auto release_name = release ? release->GetName() : context.project.GetDefaultConfiguration();

	XmlWriter xml;
	{
		XmlScope scheme(xml, "Scheme", { { "LastUpgradeVersion", "2640" }, { "version", "1.7" } });
		{
			XmlScope action(xml, "BuildAction", { { "parallelizeBuildables", "YES" }, { "buildImplicitDependencies", "YES" } });
			{
				XmlScope entries(xml, "BuildActionEntries");
				WriteBuildActionEntry(xml, context, reference);
			}
		}
		{
			XmlScope action(xml, "LaunchAction",
			{
				{ "buildConfiguration", debug_name },
				{ "selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB" },
				{ "selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB" },
				{ "launchStyle", "0" },
				{ "useCustomWorkingDirectory", "NO" },
				{ "ignoresPersistentStateOnLaunch", "NO" },
				{ "debugDocumentVersioning", "YES" },
				{ "debugServiceExtension", "internal" },
				{ "allowLocationSimulation", "YES" },
			});
			{
				XmlScope runnable(xml, "BuildableProductRunnable", { { "runnableDebuggingMode", "0" } });
				WriteBuildableReference(xml, context, reference);
			}
			WriteLaunchEnvironment(xml);
		}
		xml.EmptyElement("ProfileAction",
		{
			{ "buildConfiguration", release_name },
			{ "shouldUseLaunchSchemeArgsEnv", "YES" },
			{ "savedToolIdentifier", "" },
			{ "useCustomWorkingDirectory", "NO" },
			{ "debugDocumentVersioning", "YES" },
		});
		xml.EmptyElement("AnalyzeAction", { { "buildConfiguration", debug_name } });
		xml.EmptyElement("ArchiveAction", { { "buildConfiguration", release_name }, { "revealArchiveInOrganizer", "YES" } });
	}
	auto directory = Join(context.directory, ToWString(context.target.GetName()), L".xcodeproj/xcshareddata/xcschemes", File::kStroke);
	SaveFile(Join(directory, ToWString(context.target.GetName()), L".xcscheme"), xml.GetOutput(), context.platform.GetPlatform());
}

CString WorkspaceProjectPath(WString::View directory, const Context & context)
{
	auto project = Join(context.directory, ToWString(context.target.GetName()), L".xcodeproj");
	return EncodeUTF8(File::MakeRelativePath(directory, project));
}

void WriteWorkspaceBuildScheme(const WString & workspace, WString::View directory, ArrayView<Context> contexts)
{
	REFLEX_ASSERT(contexts);

	auto [debug, release] = FindDebugAndReleaseConfigurations(contexts[0].platform);
	auto release_name = release ? release->GetName() : contexts[0].project.GetDefaultConfiguration();

	XmlWriter xml;
	{
		XmlScope scheme(xml, "Scheme", { { "LastUpgradeVersion", "2640" }, { "version", "1.7" } });
		{
			XmlScope action(xml, "BuildAction", { { "parallelizeBuildables", "YES" }, { "buildImplicitDependencies", "YES" }, { "buildArchitectures", "Automatic" } });
			XmlScope entries(xml, "BuildActionEntries");
			for (auto & context : contexts)
			{
				auto reference = Join("container:", WorkspaceProjectPath(directory, context));
				WriteBuildActionEntry(xml, context, reference);
			}
		}
		xml.EmptyElement("TestAction",
		{
			{ "buildConfiguration", release_name },
			{ "selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB" },
			{ "selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB" },
			{ "shouldUseLaunchSchemeArgsEnv", "YES" },
			{ "shouldAutocreateTestPlan", "YES" },
		});
		{
			XmlScope action(xml, "LaunchAction",
			{
				{ "buildConfiguration", release_name },
				{ "selectedDebuggerIdentifier", "Xcode.DebuggerFoundation.Debugger.LLDB" },
				{ "selectedLauncherIdentifier", "Xcode.DebuggerFoundation.Launcher.LLDB" },
				{ "launchStyle", "0" },
				{ "useCustomWorkingDirectory", "NO" },
				{ "ignoresPersistentStateOnLaunch", "NO" },
				{ "debugDocumentVersioning", "YES" },
				{ "debugServiceExtension", "internal" },
				{ "allowLocationSimulation", "YES" },
				{ "viewDebuggingEnabled", "No" },
			});
			WriteLaunchEnvironment(xml);
		}
		xml.EmptyElement("ProfileAction",
		{
			{ "buildConfiguration", release_name },
			{ "shouldUseLaunchSchemeArgsEnv", "YES" },
			{ "savedToolIdentifier", "" },
			{ "useCustomWorkingDirectory", "NO" },
			{ "debugDocumentVersioning", "YES" },
		});
		xml.EmptyElement("AnalyzeAction", { { "buildConfiguration", release_name } });
		xml.EmptyElement("ArchiveAction", { { "buildConfiguration", release_name }, { "revealArchiveInOrganizer", "YES" } });
	}
	SaveFile(Join(workspace, L"xcshareddata/xcschemes/Build.xcscheme"), xml.GetOutput(), contexts[0].platform.GetPlatform());
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::GenerateXcodeProject(Project & project, BuildPlatform selected_platform, const WString & directory, Array<const TargetPlatform *> & generated_platforms, Map<WString> & outputs)
{
	Require(selected_platform == kBuildPlatformMacOS || selected_platform == kBuildPlatformIOS, "platform", "expected macos or ios platform");
	auto target_platforms = CollectTargetPlatforms(project, selected_platform, generated_platforms);
	Array<Xcode::Context> contexts;
	for (auto & item : target_platforms)
	{
		auto & target = *item.target;
		if (target.IsLibrary()) continue;
		Xcode::Context context = { *target.project, target, *item.platform, GetProjectFolder(*target.project, selected_platform), {}, {} };
		for (auto & dependency : target.GetDependencies(selected_platform)) if (!dependency->IsLibrary()) context.dependencies.Push(dependency.Adr());
		if (GetOutputType(*item.platform) != kOutputType_static_library)
			for (auto & dependency : item.link_dependencies) context.link_dependencies.Push(dependency.Adr());
		Xcode::ValidateTarget(context);
		contexts.Push(std::move(context));
	}
	if (contexts.Empty()) return;
	Xcode::AddOutputs(outputs, project, contexts);

	for (auto & context : contexts)
	{
		if (&context.project == &project)
		{
			Xcode::WriteProject(context);
			Xcode::WriteScheme(context);
		}
	}

	XmlWriter workspace_xml;
	{
		XmlScope workspace(workspace_xml, "Workspace", { { "version", "1.0" } });
		for (auto & context : contexts) workspace_xml.EmptyElement("FileRef", { { "location", Join("group:", Xcode::WorkspaceProjectPath(directory, context)) } });
	}
	auto workspace = Join(directory, ToWString(project.GetName()), L".xcworkspace", File::kStroke);
	SaveFile(Join(workspace, L"contents.xcworkspacedata"), workspace_xml.GetOutput(), selected_platform);
	Xcode::WriteWorkspaceBuildScheme(workspace, directory, contexts);

	Map<CString::View,bool> written_scripts;
	for (auto & context : contexts)
	{
		for (auto & config : context.platform.GetTargetConfigurations())
		{
			if (SetFiltered(written_scripts.Acquire(config->GetName()), true))
			{
				auto workspace = Join(L"\"$(dirname \"$0\")/", ToWString(project.GetName()), L".xcworkspace\"");
				auto configuration = ShellQuote(ToWString(config->GetName()));
				auto command = Template(Data::Pack(selected_platform == kBuildPlatformIOS ? Xcode::kIOSBuild : Xcode::kMacOSBuild),
				{
					{ "WORKSPACE", workspace },
					{ "CONFIGURATION", configuration },
				});
				SaveCommandScript(Join(directory, L"Build ", ToWString(config->GetName()), L'.', ReflexCLI::kCommand), command, selected_platform);
			}
		}
	}
}
