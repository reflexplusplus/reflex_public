#include "tasks.h"
#include "resources.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

// Following args are replaced directly:
// --product -> $(PRODUCT_NAME)
// --bundle_id -> $(BUNDLE_ID)
// --version -> $(VERSION)
// --vendor -> $(VENDOR_NAME)
// 
// --au_type -> $(AU_TYPE_4CC)
// --au_subtype -> $(AU_UID_4CC)
// --au_manufacturer -> $(AU_VENDOR_4CC)
// --au_description -> $(AU_DESCRIPTION)
// 
// Special cases below are generated or have fallbacks:
// $(AU_VERSION) is generated from --version as an integer.
// $(AU_TAG) is generated from --au_type.
// $(AU_DESCRIPTION) falls back to --product when omitted.

using PlistValueFn = FunctionPointer <CString(const Data::PropertySet & args, CString::View property_id)>;

using PlistBindings = Map <CString::View, Pair<CString::View, PlistValueFn>>;

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

void RegisterAppCommon(PlistBindings & bindings)
{
	bindings.Set("$(CATEGORY)", { "app_store_category", [](const Data::PropertySet & args, CString::View property_id) -> CString
	{
		return Data::GetCString(args, property_id, "music");
	} });
}

void RegisterAudioUnitCommon(PlistBindings & bindings)
{
	bindings.Set("$(VENDOR_NAME)", { "vendor", &GetString });
	bindings.Set("$(AU_TYPE_4CC)", { "au_type", &Get4CC });
	bindings.Set("$(AU_UID_4CC)", { "au_subtype", &Get4CC });
	bindings.Set("$(AU_VENDOR_4CC)", { "au_manufacturer", &Get4CC });
	bindings.Set("$(AU_DESCRIPTION)", { "au_description", [](const Data::PropertySet & args, CString::View property_id) -> CString
	{
		return Data::GetCString(args, property_id, Data::GetCString(args, "product", "Reflex Audio Plugin"));
	}});
	bindings.Set("$(AU_VERSION)", { "version", [](const Data::PropertySet & args, CString::View property_id)
	{
		auto version = GetString(args, property_id);

		auto parts = Split(version, '.');

		if (parts.GetSize() != 3) ThrowError("invalid --version", version);

		auto major = ToUInt32(parts[0]);
		auto minor = ToUInt32(parts[1]);
		auto patch = ToUInt32(parts[2]);

		return ToCString((major << 16) | (minor << 8) | patch);
	}});
}

Data::Archive PrepareBindings(Key32 target, PlistBindings & bindings)
{
	const File::EmbeddedResource * plist;

	bindings.Set("$(PRODUCT_NAME)", { "product", &GetString });
	bindings.Set("$(BUNDLE_ID)", { "bundle_id", &GetString });
	bindings.Set("$(VERSION)", { "version", &GetString });

	switch (target.value)
	{
	case MakeKey32("app"):
		RegisterAppCommon(bindings);
		plist = &app_plist;
		break;

	case MakeKey32("audioapp"):
		RegisterAppCommon(bindings);
		plist = &audioapp_plist;
		break;

	case MakeKey32("ios_app"):
		plist = &ios_app_plist;
		break;

	case MakeKey32("ios_audioapp"):
		plist = &ios_audioapp_plist;
		break;

	case MakeKey32("clap"):
	case MakeKey32("vst2"):
	case MakeKey32("vst3"):
		plist = &clap_vst_plist;
		break;

	case MakeKey32("au"):
		RegisterAudioUnitCommon(bindings);
		plist = &audiounit_plist;
		break;

	case MakeKey32("auv3"):
		RegisterAudioUnitCommon(bindings);
		bindings.Set("$(AU_TAG)", { "au_type", [](const Data::PropertySet & args, CString::View property_id) -> CString
		{
			auto au_type = GetString(args, property_id);

			switch (MakeKey32(au_type))
			{
			case MakeKey32("aumi"):
				return "MIDI";

			case MakeKey32("aumu"):
				return "Synthesizer";

			default:
				return "Effects";
			}
		}});
		plist = &auv3_plist;
		break;

	default:
		ThrowError("unknown target", "?");
		break;
	}

	return plist->data;
}

REFLEX_END_INTERNAL

void ReflexCLI::BuildPlist(const Data::PropertySet & args, System::FileHandle & std_out)
{
	Bootstrap::CLI::RequireArgs(args, { "target", "output" });

	auto target = Bootstrap::CLI::GetString(args, "target");

	auto filename = Bootstrap::CLI::GetFilenameArg(args, "output", false);

	PlistBindings bindings;

	auto blob = PrepareBindings(target, bindings);

	for (auto & [token,binding] : bindings)
	{
		output.Log(token);
	
		CString value = binding.b(args, binding.a);

		blob = Replace(blob, Data::Pack(token), Data::Pack(value));
	}

	if (!File::Save(filename, blob)) ThrowError("failed to write plist", filename);

	File::WriteLine(std_out, filename);
}
