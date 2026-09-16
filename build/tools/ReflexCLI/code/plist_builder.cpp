#include "tasks.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

CString GetString(const Data::PropertySet & args, CString::View property_id)
{
	auto value = Data::GetCString(args, property_id);

	if (!value)
	{
		Bootstrap::CLI::ThrowMissingArg(property_id, "<value>");
	}

	return value;
}

CString Get4CC(const Data::PropertySet & args, CString::View property_id)
{
	auto value = GetString(args, property_id);

	if (value.GetSize() != 4)
	{
		ThrowError("invalid 4cc value", value);
	}

	return value;
}

UInt32 GetAudioUnitVersion(const Data::PropertySet & args)
{
	auto version = GetString(args, "version");
	auto parts = Split(version, '.');

	if (parts.GetSize() != 3) ThrowError("invalid --version", version);

	auto major = ToUInt32(parts[0]);
	auto minor = ToUInt32(parts[1]);
	auto patch = ToUInt32(parts[2]);

	return (major << 16) | (minor << 8) | patch;
}

struct AudioUnitComponent
{
	CString subtype;
	CString type;
	CString name;
};

Array <AudioUnitComponent> GetAudioUnitComponents(const Data::PropertySet & args)
{
	auto product = GetString(args, "product");
	auto value = GetString(args, "au_components");
	Array <AudioUnitComponent> components;

	for (auto entry : Split(value, ','))
	{
		auto parts = Split(entry, ':');
		if (parts.GetSize() < 2 || parts.GetSize() > 3 || !parts[0] || !parts[1]) ThrowError("invalid --au_components entry", entry);

		AudioUnitComponent component;
		component.subtype = parts[0];
		component.type = parts[1];
		if (parts.GetSize() == 3) component.name = parts[2];
		else component.name = product;

		if (component.subtype.GetSize() != 4 || component.type.GetSize() != 4 || !component.name) ThrowError("invalid --au_components entry", entry);

		switch (MakeKey32(component.type))
		{
		case MakeKey32("aufx"):
		case MakeKey32("aumu"):
		case MakeKey32("aumi"):
			break;

		default:
			ThrowError("unsupported AU component type", component.type);
		}

		for (const auto & existing : components)
		{
			if (existing.subtype == component.subtype && existing.type == component.type) ThrowError("duplicate --au_components entry", entry);
		}

		components.Push(std::move(component));
	}

	if (!components) ThrowError("invalid --au_components", value);

	return components;
}

void WriteKey(XmlWriter & xml, CString::View key)
{
	xml.Element("key", key, true);
}

void WriteString(XmlWriter & xml, CString::View key, CString::View value)
{
	WriteKey(xml, key);
	xml.Element("string", value, true);
}

void WriteInteger(XmlWriter & xml, CString::View key, UInt32 value)
{
	WriteKey(xml, key);
	xml.Element("integer", ToCString(value));
}

void WriteBool(XmlWriter & xml, CString::View key, bool value)
{
	WriteKey(xml, key);
	xml.EmptyElement(value ? "true" : "false");
}

void WriteCommonBundleKeys(XmlWriter & xml, CString::View product, CString::View executable, CString::View bundle_id, CString::View version, CString::View package_type, bool signature)
{
	WriteString(xml, "CFBundleDevelopmentRegion", "English");
	WriteString(xml, "CFBundleExecutable", executable);
	WriteString(xml, "CFBundleIdentifier", bundle_id);
	WriteString(xml, "CFBundleInfoDictionaryVersion", "6.0");
	WriteString(xml, "CFBundleName", product);
	WriteString(xml, "CFBundlePackageType", package_type);
	WriteString(xml, "CFBundleShortVersionString", version);
	if (signature) WriteString(xml, "CFBundleSignature", "????");
	WriteString(xml, "CFBundleVersion", version);
}

void WriteMacAppKeys(XmlWriter & xml, const Data::PropertySet & args, bool audio_app)
{
	auto product = GetString(args, "product");
	auto executable = Data::GetCString(args, "executable", product);
	WriteCommonBundleKeys(xml, product, executable, GetString(args, "bundle_id"), GetString(args, "version"), "APPL", true);

	WriteKey(xml, "NSAppTransportSecurity");
	{
		XmlScope transport_security(xml, "dict");
		WriteBool(xml, "NSAllowsArbitraryLoads", true);
	}

	WriteString(xml, "NSPrincipalClass", "NSApplication");
	WriteString(xml, "LSApplicationCategoryType", Join("public.app-category.", Data::GetCString(args, "app_store_category", "music")));
	if (audio_app) WriteString(xml, "NSMicrophoneUsageDescription", "Audio Input");
}

void WriteIosSceneManifest(XmlWriter & xml)
{
	WriteKey(xml, "UIApplicationSceneManifest");
	XmlScope scene_manifest(xml, "dict");
	WriteBool(xml, "UIApplicationSupportsMultipleScenes", false);
	WriteKey(xml, "UISceneConfigurations");
	XmlScope scene_configurations(xml, "dict");
	WriteKey(xml, "UIWindowSceneSessionRoleApplication");
	XmlScope application_role(xml, "array");
	XmlScope configuration(xml, "dict");
	WriteString(xml, "UISceneConfigurationName", "Default Configuration");
	WriteString(xml, "UISceneDelegateClassName", "ReflexSceneDelegate");
}

void WriteIosAppKeys(XmlWriter & xml, const Data::PropertySet & args, bool audio_app)
{
	auto product = GetString(args, "product");
	auto executable = Data::GetCString(args, "executable", product);
	WriteCommonBundleKeys(xml, product, executable, GetString(args, "bundle_id"), GetString(args, "version"), "APPL", false);
	WriteBool(xml, "LSRequiresIPhoneOS", true);
	WriteIosSceneManifest(xml);

	WriteKey(xml, "UIRequiredDeviceCapabilities");
	{
		XmlScope capabilities(xml, "array");
		xml.Element("string", "arm64");
	}

	if (audio_app)
	{
		WriteString(xml, "NSMicrophoneUsageDescription", "Audio Input");
		WriteKey(xml, "UIBackgroundModes");
		XmlScope modes(xml, "array");
		xml.Element("string", "audio");
	}
}

void WritePluginKeys(XmlWriter & xml, const Data::PropertySet & args)
{
	auto product = GetString(args, "product");
	auto executable = Data::GetCString(args, "executable", product);
	WriteCommonBundleKeys(xml, product, executable, GetString(args, "bundle_id"), GetString(args, "version"), "BNDL", true);
}

void WriteAudioUnitComponent(XmlWriter & xml, const Data::PropertySet & args, const AudioUnitComponent & component, bool auv3)
{
	auto vendor = GetString(args, "vendor");

	XmlScope component_dict(xml, "dict");

	if (!auv3)
	{
		WriteString(xml, "description", component.name);
		WriteString(xml, "factoryFunction", "AudioUnitFactory");
	}

	WriteString(xml, "manufacturer", Get4CC(args, "au_manufacturer"));
	WriteString(xml, "name", Join(vendor, ": ", component.name));
	WriteString(xml, "subtype", component.subtype);
	WriteString(xml, "type", component.type);
	WriteInteger(xml, "version", GetAudioUnitVersion(args));

	if (auv3)
	{
		WriteBool(xml, "sandboxSafe", true);
		WriteKey(xml, "tags");
		XmlScope tags(xml, "array");

		switch (MakeKey32(component.type))
		{
		case MakeKey32("aumi"):
			xml.Element("string", "MIDI");
			break;

		case MakeKey32("aumu"):
			xml.Element("string", "Synthesizer");
			break;

		default:
			xml.Element("string", "Effects");
			break;
		}
	}
}

void WriteAudioUnitKeys(XmlWriter & xml, const Data::PropertySet & args)
{
	WriteKey(xml, "AudioComponents");
	{
		XmlScope components(xml, "array");
		for (const auto & component : GetAudioUnitComponents(args)) WriteAudioUnitComponent(xml, args, component, false);
	}

	WritePluginKeys(xml, args);
	WriteBool(xml, "NSHighResolutionCapable", true);
}

void WriteAudioUnitV3Keys(XmlWriter & xml, const Data::PropertySet & args)
{
	auto product = GetString(args, "product");
	auto executable = Data::GetCString(args, "executable", product);
	WriteCommonBundleKeys(xml, product, executable, GetString(args, "bundle_id"), GetString(args, "version"), "XPC!", false);
	WriteString(xml, "CFBundleDisplayName", product);

	WriteKey(xml, "NSExtension");
	XmlScope extension(xml, "dict");
	WriteString(xml, "NSExtensionPointIdentifier", "com.apple.AudioUnit-UI");
	WriteString(xml, "NSExtensionPrincipalClass", "ReflexAuv3ViewController");
	WriteKey(xml, "NSExtensionAttributes");
	XmlScope attributes(xml, "dict");
	WriteKey(xml, "AudioComponents");
	XmlScope components(xml, "array");
	for (const auto & component : GetAudioUnitComponents(args)) WriteAudioUnitComponent(xml, args, component, true);
}

Data::Archive GeneratePlist(const Data::PropertySet & args, Key32 target)
{
	XmlWriter xml("plist", "-//Apple//DTD PLIST 1.0//EN", "http://www.apple.com/DTDs/PropertyList-1.0.dtd");

	{
		XmlScope plist(xml, "plist", { { "version", "1.0" } });
		XmlScope dictionary(xml, "dict");

		switch (target.value)
		{
		case MakeKey32("app"):
			WriteMacAppKeys(xml, args, false);
			break;

		case MakeKey32("audioapp"):
			WriteMacAppKeys(xml, args, true);
			break;

		case MakeKey32("ios_app"):
			WriteIosAppKeys(xml, args, false);
			break;

		case MakeKey32("ios_audioapp"):
			WriteIosAppKeys(xml, args, true);
			break;

		case MakeKey32("clap"):
		case MakeKey32("vst2"):
		case MakeKey32("vst3"):
			WritePluginKeys(xml, args);
			break;

		case MakeKey32("au"):
			WriteAudioUnitKeys(xml, args);
			break;

		case MakeKey32("auv3"):
			WriteAudioUnitV3Keys(xml, args);
			break;

		default:
			ThrowError("unknown target", "?");
			break;
		}
	}

	return xml.GetOutput();
}

REFLEX_END_INTERNAL

void ReflexCLI::BuildPlist(const Data::PropertySet & args, System::FileHandle & std_out)
{
	Bootstrap::CLI::RequireArgs(args, { "target", "output" });

	auto target = Bootstrap::CLI::GetString(args, "target");
	auto filename = Bootstrap::CLI::GetFilename(args, "output", false);
	auto blob = GeneratePlist(args, target);

	File::MakePath(File::SplitFilename(filename).a);

	if (!SaveGeneratedFile(filename, blob)) ThrowError("failed to write plist", filename);

	File::WriteLine(std_out, filename);
}
