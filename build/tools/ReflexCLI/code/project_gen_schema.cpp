#include "project_gen.h"

REFLEX_NS(ReflexCLI::ProjectGen)

struct PropertyRule;

using PropertyHandler = void (*)(ValidatedPropertySet &, const PropertyRule &, Address, Object &);

struct PropertyRule
{
	Address address;
	PropertyHandler handler;
	PropertySetType property_set_type = kPropertySetTypeVariables;
};

struct PropertyRules
{
	Array<PropertyRule> known_properties;
	Array<PropertyRule> free_properties;
};

struct Unset : public Object
{
	REFLEX_OBJECT(Unset, Object);
	static Unset & null;
};

struct AppendingArray : public Object
{
	REFLEX_OBJECT(AppendingArray, Object);
	static AppendingArray & null;

	AppendingArray() = default;
	AppendingArray(TypeID type, Object & values)
		: type(type)
		, values(values)
	{
	}

	TypeID type = {};
	Reference<Object, false> values;
};

REFLEX_END

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen)

struct PropertySheetInterface;

UInt * GetPropertySheetLine();

void InvalidStructure(CString::View description)
{
	auto line = GetPropertySheetLine();
	REFLEX_ASSERT(line);
	Data::Detail::TokeniseFail(*line, description);
}

void InvalidStructure(Object & object, CString::View description)
{
	auto release = AutoRelease(object);
	InvalidStructure(description);
}

void RequireStructure(bool test, CString::View description)
{
	if (!test) InvalidStructure(description);
}

void RequireStructure(bool test, Object & object, CString::View description)
{
	if (!test) InvalidStructure(object, description);
}

Object * QueryLocalProperty(Data::PropertySet & values, Address address)
{
	for (auto & item : values.Iterate()) if (item.key == address) return item.value.Adr();
	return nullptr;
}

bool IsUnset(Address address)
{
	return address.type_id == GetTypeID<Unset>();
}

void ValidateFiles(const Data::PropertySet & values, Object & owner)
{
	auto string_t = GetTypeID<Data::CStringProperty>();
	auto array_string_t = GetTypeID<Data::ArrayOfCStringProperty>();

	for (auto & [adr,value] : values.Iterate())
	{
		if (adr.id == MakeKey32(kInherit)) InvalidStructure(owner, "inherit is reserved in Files");
		if (IsUnset(adr)) continue;
		if (adr.type_id == string_t || adr.type_id == array_string_t) continue;
		if (auto child = DynamicCast<Data::PropertySet>(value))
		{
			ValidateFiles(*child, owner);
			continue;
		}
		InvalidStructure(owner, "Files values must be strings, arrays of strings, or nested groups");
	}
}

Reference<Data::PropertySet> ClonePropertySet(const Data::PropertySet & source, Data::PropertySet * parent = nullptr);

void AppendArray(Object & target, const Object & source, TypeID type)
{
	if (type == GetTypeID<Data::ArrayOfCStringProperty>()) Cast<Data::ArrayOfCStringProperty>(target)->value.Append(Cast<Data::ArrayOfCStringProperty>(source)->value);
	else if (type == GetTypeID<Data::ArrayOfKey32Property>()) Cast<Data::ArrayOfKey32Property>(target)->value.Append(Cast<Data::ArrayOfKey32Property>(source)->value);
	else if (type == GetTypeID<Data::PropertySetArray>()) Cast<Data::PropertySetArray>(target)->value.Append(Cast<Data::PropertySetArray>(source)->value);
	else REFLEX_ASSERT(false);
}

Reference<AppendingArray> CloneAppendingArray(const AppendingArray & source)
{
	Reference<Object> values;
	if (source.type == GetTypeID<Data::ArrayOfCStringProperty>()) values = Make<Data::ArrayOfCStringProperty>(Cast<Data::ArrayOfCStringProperty>(*source.values)->value);
	else if (source.type == GetTypeID<Data::ArrayOfKey32Property>()) values = Make<Data::ArrayOfKey32Property>(Cast<Data::ArrayOfKey32Property>(*source.values)->value);
	else if (source.type == GetTypeID<Data::PropertySetArray>()) values = Make<Data::PropertySetArray>(Cast<Data::PropertySetArray>(*source.values)->value);
	else REFLEX_ASSERT(false);
	return Make<AppendingArray>(source.type, *values);
}

void MergeAppendingArray(AppendingArray & target, const AppendingArray & source)
{
	REFLEX_ASSERT(target.type == source.type);
	AppendArray(*target.values, *source.values, target.type);
}

void RemovePropertiesWithID(Data::PropertySet & values, Key32 id)
{
	Array<Address> removed;
	for (auto & [adr, value] : values.Iterate()) if (adr.id == id) removed.Push(adr);
	for (auto address : removed) values.UnsetProperty(address);
}

bool HasUnset(const Data::PropertySet & values, Key32 id)
{
	for (auto & [adr, value] : values.Iterate()) if (adr.id == id && IsUnset(adr)) return true;
	return false;
}

void StoreProperty(Data::PropertySet & values, Address address, Object & object)
{
	if (auto validated = DynamicCast<ValidatedPropertySet>(values)) validated->StoreProperty(address, object);
	else values.SetProperty(address, object);
}

void CopyInheritedProperties(Data::PropertySet & target, const Data::PropertySet & source)
{
	auto keymap_t = GetTypeID<Data::KeyMap>();
	auto appending_array_t = GetTypeID<AppendingArray>();

	// Apply reset markers before values so @Unset id; id: value; is independent
	// of the PropertySet's address ordering.
	for (auto & [adr, value] : source.Iterate())
	{
		if (!IsUnset(adr)) continue;
		RemovePropertiesWithID(target, adr.id);
		StoreProperty(target, adr, value);
	}

	for (auto & [adr, value] : source.Iterate())
	{
		REFLEX_ASSERT(adr.id != MakeKey32(kInherit));//shouldnt be stored
		//if (adr.id == MakeKey32(kInherit)) continue;
		if (IsUnset(adr)) continue;
		if (adr.type_id == keymap_t)
		{
			Data::Assimilate(Data::AcquireKeyMap(target), Cast<Data::KeyMap>(value));
			continue;
		}
		if (adr.type_id == appending_array_t)
		{
			auto source_array = Cast<AppendingArray>(value);
			if (auto target_object = QueryLocalProperty(target, adr)) MergeAppendingArray(*Cast<AppendingArray>(*target_object), *source_array);
			else
			{
				auto copy = CloneAppendingArray(*source_array);
				StoreProperty(target, adr, copy);
			}
			continue;
		}

		if (auto source_values = DynamicCast<Data::PropertySet>(value))
		{
			if (auto existing = QueryLocalProperty(target, adr))
			{
				if (auto target_values = DynamicCast<Data::PropertySet>(*existing))
				{
					CopyInheritedProperties(*target_values, *source_values);
					continue;
				}
			}
			auto copy = ClonePropertySet(*source_values, &target);
			StoreProperty(target, adr, copy);
			continue;
		}

		Array<Address> replaced;
		for (auto & current : target.Iterate())
		{
			if (current.key.id != adr.id) continue;
			if (IsUnset(current.key) && HasUnset(source, adr.id)) continue;
			replaced.Push(current.key);
		}
		for (auto address : replaced) target.UnsetProperty(address);
		StoreProperty(target, adr, value);
	}
}

Reference<Data::PropertySet> ClonePropertySet(const Data::PropertySet & source, Data::PropertySet * parent)
{
	Reference<Data::PropertySet> result;
	if (auto value = DynamicCast<ValidatedPropertySet>(source))
	{
		const ValidatedPropertySet * driver = nullptr;
		if (value->type > kPropertySetTypeBuildStep)
		{
			REFLEX_ASSERT(parent);
			driver = value;
		}
		switch (value->type)
		{
		case kPropertySetTypeVariables:
		case kPropertySetTypeFiles:
		case kPropertySetTypeBuildStep:
			result = Make<ValidatedPropertySet>(value->type); 
			break;

		case kPropertySetTypeTemplate:
		case kPropertySetTypeTarget:
		case kPropertySetTypeLibrary:
		case kPropertySetTypeConfiguration: 
			result = Make<ValidatedPropertySet>(value->type, *driver->keymap, *parent, driver->id); 
			break;

		case kPropertySetTypePlatform:
			result = Make<PlatformPropertySet>(*driver->keymap, *parent, Cast<PlatformPropertySet>(value)->GetPlatform()); 
			break;
		
		default:
			REFLEX_ASSERT(false); 
			result = Make<Data::PropertySet>(); 
			break;
		}
	}
	else
	{
		result = Make<Data::PropertySet>();
	}

	for (auto & item : source.Iterate())
	{
		if (auto source_array = DynamicCast<AppendingArray>(item.value))
		{
			auto copy = CloneAppendingArray(*source_array);
			if (auto target_values = DynamicCast<ValidatedPropertySet>(*result)) target_values->StoreProperty(item.key, copy);
			else result->SetProperty(item.key, copy);
		}
		else if (auto child = DynamicCast<Data::PropertySet>(item.value))
		{
			auto copy = ClonePropertySet(*child, result.Adr());
			if (auto target_values = DynamicCast<ValidatedPropertySet>(*result)) target_values->StoreProperty(item.key, copy);
			else result->SetProperty(item.key, copy);
		}
		else
		{
			if (auto target_values = DynamicCast<ValidatedPropertySet>(*result)) target_values->StoreProperty(item.key, item.value);
			else result->SetProperty(item.key, item.value);
		}
	}
	return result;
}

struct PropertySheetInterface : public Data::Detail::PropertySheetInterface
{
	static UInt * st_line;

	static void SetLine(Context & context)
	{
		st_line = &context.line;
	}

	Object * OnBegin(Data::PropertySet & root) const override
	{
		st_line = nullptr;
		auto driver = DynamicCast<ValidatedPropertySet>(root);
		REFLEX_ASSERT(driver && driver->IsRoot());
		auto keymap = driver->keymap.RemoveConst();
		REFLEX_ASSERT(IsValid(keymap));
		root.SetProperty(Data::kkeymap, keymap);
		return keymap.Adr();
	}

	void OnEnd(Context & context, Data::PropertySet & root) const override
	{
		SetLine(context);
		st_line = nullptr;
	}

	bool OnSetOption(Context & context, Key32, const CString::View &) const override
	{
		SetLine(context);
		return false;
	}

	ObjectWithType CreateObject(Context & context, Object & parent, const CString::View & type, Key32 id, bool is_stub) const override
	{
		SetLine(context);
		const Key32 type_id = type;

		if (!type)
		{
			return Data::Detail::g_standard_propertysheet_interface->CreateObject(context, parent, type, id, is_stub);
		}

		if (is_stub)
		{
			if (type == "Unset")
			{
				RequireStructure(IsSet(id), "Unset requires a property name");
				return Data::Detail::MakeObjectWithType<Unset>();
			}
			RequireStructure(type_id == K32("Configuration"), "typed objects must have a body");
		}

		auto & keymap = context.keymap;

		auto RequireNamed = [id]()
		{
			RequireStructure(IsSet(id), "typed object requires a name");
		};

		auto RequireAnonymous = [id]()
		{
			RequireStructure(IsUnset(id), "platform object must not have a name");
		};

		auto CheckParent = [](Object & parent, PropertySetType type)
		{
			auto root = DynamicCast<ValidatedPropertySet>(parent);
			return (root && root->type == type);
		};

		switch (type_id.value)
		{
		case K32("Variables"):
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeVariables);

		case K32("Files"):
			RequireNamed();
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeFiles);

		case K32("BuildStep"):
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeBuildStep);

		case K32("Template"):
			RequireNamed();
			RequireStructure(CheckParent(parent, kPropertySetTypeProject), "Template is only valid at project scope");
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeTemplate, keymap, Cast<Data::PropertySet>(parent), id);

		case K32("Target"):
			RequireNamed();
			RequireStructure(CheckParent(parent, kPropertySetTypeProject), "Target is only valid at project scope");
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeTarget, keymap, Cast<Data::PropertySet>(parent), id);

		case K32("Library"):
			RequireNamed();
			RequireStructure(CheckParent(parent, kPropertySetTypeProject), "Library is only valid at project scope");
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeLibrary, keymap, Cast<Data::PropertySet>(parent), id);

		case K32("windows"):
			RequireAnonymous();
			return CreatePlatform(keymap, parent, kBuildPlatformWindows);

		case K32("macos"):
			RequireAnonymous();
			return CreatePlatform(keymap, parent, kBuildPlatformMacOS);

		case K32("ios"):
			RequireAnonymous();
			return CreatePlatform(keymap, parent, kBuildPlatformIOS);

		case K32("android"):
			RequireAnonymous();
			return CreatePlatform(keymap, parent, kBuildPlatformAndroid);

		case K32("linux"):
			RequireAnonymous();
			return CreatePlatform(keymap, parent, kBuildPlatformLinux);

		case K32("Configuration"):
			RequireNamed();
			RequireStructure(CheckParent(parent, kPropertySetTypePlatform), "Configuration is only valid in a platform");
			return Data::Detail::CreateObjectWithType<ValidatedPropertySet>(kPropertySetTypeConfiguration, keymap, Cast<Data::PropertySet>(parent), id);

		default:
			InvalidStructure(Join("unknown structural type ", type));
			return {};
		}
	}

	ObjectWithType CreateObjectArray(Context & context, const CString::View & type, const Array <ObjectWithType> & objects) const override
	{
		SetLine(context);
		if (MakeKey32(type) == K32("BuildStep"))
		{
			auto result = New<Data::PropertySetArray>();
			result->value.Allocate(objects.GetSize());
			for (auto & object : objects)
			{
				auto value = DynamicCast<ValidatedPropertySet>(object.a);
				RequireStructure(value && value->type == kPropertySetTypeBuildStep, "BuildStep arrays may only contain BuildStep objects");
				result->value.Push(Cast<Data::PropertySet>(object.a));
			}
			return Data::Detail::MakeObjectWithType(*result);
		}

		for (auto & object : objects)
		{
			if (auto value = DynamicCast<ValidatedPropertySet>(object.a); value && value->type > kPropertySetTypeBuildStep) InvalidStructure("structural objects cannot be array elements");
		}

		return Data::Detail::g_standard_propertysheet_interface->CreateObjectArray(context, type, objects);
	}

	ObjectWithType CreateValue(Context & context, const CString::View & type, TokenType token_t, const CString::View & value) const override
	{
		SetLine(context);
		return Data::Detail::g_standard_propertysheet_interface->CreateValue(context, type, token_t, value);
	}

	ObjectWithType CreateValueArray(Context & context, const CString::View & type, TokenType token_t, const Array <CString::View> & values) const override
	{
		SetLine(context);
		return Data::Detail::g_standard_propertysheet_interface->CreateValueArray(context, type, token_t, values);
	}

private:

	ObjectWithType CreatePlatform(const Data::KeyMap & keymap, Object & parent, BuildPlatform platform) const
	{
		auto values = DynamicCast<ValidatedPropertySet>(parent);
		RequireStructure(values && (values->type == kPropertySetTypeTemplate || values->type == kPropertySetTypeTarget || values->type == kPropertySetTypeLibrary), "platform is only valid in a Template, Target, or Library");

		return Data::Detail::CreateObjectWithType<PlatformPropertySet>(keymap, Cast<Data::PropertySet>(parent), platform);
	}
};

UInt * PropertySheetInterface::st_line = nullptr;

UInt * GetPropertySheetLine()
{
	return PropertySheetInterface::st_line;
}

void HandleSet(ValidatedPropertySet & owner, const PropertyRule &, Address address, Object & object)
{
	owner.StoreProperty(address, object);
}

void HandleVariable(ValidatedPropertySet & owner, const PropertyRule & rule, Address address, Object & object)
{
	RequireStructure(address.id != MakeKey32(kInherit), object, "inherit is reserved in Variables");
	HandleSet(owner, rule, address, object);
}

template <class VALUE> struct ListValue
{
	static auto Get(Object & object) { return Cast<VALUE>(object)->value; }
};

template <> struct ListValue<Data::PropertySet>
{
	static auto Get(Object & object) { return Cast<Data::PropertySet>(object); }
};

template <class VALUE, class ARRAY> void HandleList(ValidatedPropertySet & owner, const PropertyRule & rule, Address address, Object & object)
{
	if (address.type_id == GetTypeID<ARRAY>())
	{
		owner.StoreProperty(address, object);
	}
	else
	{
		auto release = AutoRelease(object);
		Address target = MakeAddress<ARRAY>(rule.address.id);
		auto values = New<ARRAY>();
		values->value.Push(ListValue<VALUE>::Get(object));
		owner.StoreProperty(target, values);
	}
}

template <class VALUE, class ARRAY> void HandleAppendingList(ValidatedPropertySet & owner, const PropertyRule & rule, Address address, Object & object)
{
	auto release = AutoRelease(object);
	Reference<ARRAY> values;
	if (address.type_id == GetTypeID<ARRAY>()) values = Cast<ARRAY>(object);
	else
	{
		values = Make<ARRAY>();
		values->value.Push(ListValue<VALUE>::Get(object));
	}

	auto id = rule.address.id;
	auto target_address = MakeAddress<AppendingArray>(id);
	if (auto existing = QueryLocalProperty(owner, target_address))
	{
		auto target = Cast<AppendingArray>(*existing);
		RequireStructure(target->type == GetTypeID<ARRAY>(), "incompatible appending array type");
		AppendArray(*target->values, *values, target->type);
		return;
	}
	auto wrapped = New<AppendingArray>(GetTypeID<ARRAY>(), *values);
	owner.StoreProperty(target_address, wrapped);
}

void HandleMergePropertySet(ValidatedPropertySet & owner, const PropertyRule &, Address address, Object & object)
{
	address.type_id = GetTypeID<Data::PropertySet>();
	auto & incoming = *Cast<Data::PropertySet>(object);
	if (auto existing = QueryLocalProperty(owner, address))
	{
		if (existing->object_t != incoming.object_t) InvalidStructure(object, "cannot overlay incompatible property map types");
		CopyInheritedProperties(*Cast<Data::PropertySet>(existing), incoming);
		auto release = AutoRelease(object);
		return;
	}
	owner.StoreProperty(address, object);
}

void RequirePropertySetType(Object & object, PropertySetType type)
{
	if (Cast<ValidatedPropertySet>(object)->type != type) InvalidStructure(object, "invalid property value");
}

void HandleMap(ValidatedPropertySet & owner, const PropertyRule & rule, Address address, Object & object)
{
	RequirePropertySetType(object, rule.property_set_type);
	HandleMergePropertySet(owner, rule, address, object);
}

void HandleBuildStep(ValidatedPropertySet & owner, const PropertyRule & rule, Address address, Object & object)
{
	RequirePropertySetType(object, kPropertySetTypeBuildStep);
	HandleAppendingList<Data::PropertySet,Data::PropertySetArray>(owner, rule, address, object);
}

void HandleValidateFiles(ValidatedPropertySet & owner, const PropertyRule &, Address address, Object & object)
{
	ValidateFiles(*Cast<Data::PropertySet>(object), object);
	owner.StoreProperty(address, object);
}

void HandleReject(ValidatedPropertySet &, const PropertyRule &, Address, Object & object)
{
	InvalidStructure(object, "invalid property value");
}

void HandleInherit(ValidatedPropertySet & owner, const PropertyRule &, Address address, Object & object)
{
	auto & values = *Cast<ValidatedPropertySet>(owner);
	ArrayView<Key32> names;
	if (address.type_id == GetTypeID<Data::Key32Property>()) names = ToView(Cast<Data::Key32Property>(object)->value);
	else names = Cast<Data::ArrayOfKey32Property>(object)->value;
	RequireStructure(names.size, object, "inherit requires at least one template");

	Array<Key32> inherited;
	for (auto name : names)
	{
		if (Search(inherited, name)) InvalidStructure(object, "duplicate inherited template");
		inherited.Push(name);

		auto root = DynamicCast<ValidatedPropertySet>(*values.parent);
		REFLEX_ASSERT(root && root->type == kPropertySetTypeProject);
		auto project = static_cast<const Project *>(root);
		auto match = project->FindTemplate(name);
		RequireStructure(match, object, "inherited template must be declared before use");
		CopyInheritedProperties(values, *match);
	}
	auto release = AutoRelease(object);
}

void HandleStructuralChild(ValidatedPropertySet & owner, const PropertyRule &, Address address, Object & object)
{
	auto & values = *Cast<ValidatedPropertySet>(owner);
	auto & child = *Cast<ValidatedPropertySet>(object);
	bool valid = (values.type == kPropertySetTypeProject
		&& (child.type == kPropertySetTypeTemplate || child.type == kPropertySetTypeTarget || child.type == kPropertySetTypeLibrary))
		|| ((values.type == kPropertySetTypeTemplate || values.type == kPropertySetTypeTarget || values.type == kPropertySetTypeLibrary)
			&& child.type == kPropertySetTypePlatform)
		|| (values.type == kPropertySetTypePlatform && child.type == kPropertySetTypeConfiguration);
	RequireStructure(valid, object, "invalid structural child");
	REFLEX_ASSERT(child.parent.Adr() == &values);
	if (auto platform = DynamicCast<PlatformPropertySet>(child)) address.id = platform->id;
	if (auto existing = QueryLocalProperty(values, address))
	{
		CopyInheritedProperties(*Cast<ValidatedPropertySet>(existing), child);
		auto release = AutoRelease(object);
		return;
	}
	owner.StoreProperty(address, object);
}

struct FormatHolder
{
	template <class TYPE> static void AddKnown(PropertyRules & rules, Key32 id, PropertyHandler handler = &HandleSet, bool appendable = false, PropertySetType property_set_type = kPropertySetTypeVariables)
	{
		rules.known_properties.Push({ MakeAddress<TYPE>(id), handler, property_set_type });
	}

	template <class TYPE> static void AddFree(PropertyRules & rules, PropertyHandler handler = &HandleSet)
	{
		rules.free_properties.Push({ MakeAddress<TYPE>({}), handler });
	}

	static void AddCStringList(PropertyRules & rules, Key32 id, bool appendable = true)
	{
		AddKnown<Data::CStringProperty>(rules, id, &HandleList<Data::CStringProperty, Data::ArrayOfCStringProperty>, appendable);
		AddKnown<Data::ArrayOfCStringProperty>(rules, id, &HandleList<Data::CStringProperty, Data::ArrayOfCStringProperty>, appendable);
	}

	static void AddKeyList(PropertyRules & rules, Key32 id, bool appendable = true)
	{
		AddKnown<Data::Key32Property>(rules, id, &HandleList<Data::Key32Property, Data::ArrayOfKey32Property>, appendable);
		AddKnown<Data::ArrayOfKey32Property>(rules, id, &HandleList<Data::Key32Property, Data::ArrayOfKey32Property>, appendable);
	}

	static void AddAppendingKeyList(PropertyRules & rules, Key32 id)
	{
		AddKnown<Data::Key32Property>(rules, id, &HandleAppendingList<Data::Key32Property, Data::ArrayOfKey32Property>);
		AddKnown<Data::ArrayOfKey32Property>(rules, id, &HandleAppendingList<Data::Key32Property, Data::ArrayOfKey32Property>);
	}

	static void AddAppendingCStringList(PropertyRules & rules, Key32 id)
	{
		AddKnown<Data::CStringProperty>(rules, id, &HandleAppendingList<Data::CStringProperty, Data::ArrayOfCStringProperty>);
		AddKnown<Data::ArrayOfCStringProperty>(rules, id, &HandleAppendingList<Data::CStringProperty, Data::ArrayOfCStringProperty>);
	}

	static void AddFiles(PropertyRules & rules, Key32 id)
	{
		AddCStringList(rules, id);
		AddKnown<Data::PropertySet>(rules, id);
		AddKnown<ValidatedPropertySet>(rules, id, &HandleMap, false, kPropertySetTypeFiles);
	}

	static void AddVariables(PropertyRules & rules, Key32 id)
	{
		AddKnown<ValidatedPropertySet>(rules, id, &HandleMap, false, kPropertySetTypeVariables);
	}

	static void AddBuildSteps(PropertyRules & rules, Key32 id)
	{
		AddKnown<ValidatedPropertySet>(rules, id, &HandleBuildStep, true);
		AddKnown<Data::PropertySetArray>(rules, id, &HandleAppendingList<Data::PropertySet, Data::PropertySetArray>, true);
	}

	static void Append(PropertyRules & target, const PropertyRules & source)
	{
		target.known_properties.Append(source.known_properties);
		target.free_properties.Append(source.free_properties);
	}

	static void AddCommon(PropertyRules & rules)
	{
		AddKnown<Data::BoolProperty>(rules, kEnabled);
		AddVariables(rules, kNullKey);
		AddKnown<Data::CStringProperty>(rules, kOutputName);
		AddKnown<Data::Key32Property>(rules, kOutputType);
		AddKnown<Data::CStringProperty>(rules, kOutputExtension);
		AddKnown<Data::CStringProperty>(rules, kPath);
		AddKeyList(rules, kArchitectures);
		AddKnown<Data::CStringProperty>(rules, kOutputDirectory);
		AddKnown<Data::CStringProperty>(rules, kIntermediateDirectory);
		AddKnown<Data::Key32Property>(rules, kCppStandard);
		AddKnown<Data::BoolProperty>(rules, kRtti);
		AddVariables(rules, kDefines);
		AddAppendingCStringList(rules, kIncludeDirectories);
		AddCStringList(rules, kCompilerOptions);
		AddKnown<Data::Key32Property>(rules, kWarningLevel);
		AddKnown<Data::Key32Property>(rules, kOptimization);
		AddKnown<Data::Key32Property>(rules, kFloatingPoint);
		AddKnown<Data::Key32Property>(rules, kRuntimeLibrary);
		AddAppendingKeyList(rules, kDependencies);
		AddKnown<Data::BoolProperty>(rules, kDebugInformation);
		AddKnown<Data::BoolProperty>(rules, kDeadStrip);
		AddKnown<Data::CStringProperty>(rules, kCMakeIdentifier);
		AddVariables(rules, kCMakeFindPackages);
		AddCStringList(rules, kCMakeIncludes);
		AddVariables(rules, kCMakeVariables);
		AddFiles(rules, kSources);
		AddFiles(rules, kHeaders);
		AddFiles(rules, kOtherFiles);
		for (auto phase : kBuildPhases) AddBuildSteps(rules, phase);
	}

	static void AddPlatform(PropertyRules & rules, BuildPlatform platform)
	{
		switch (platform)
		{
		case kBuildPlatformWindows:
			AddKnown<Data::CStringProperty>(rules, kWindowsSdk);
			AddFiles(rules, kWindowsResources);
			break;

		case kBuildPlatformMacOS:
		case kBuildPlatformIOS:
		{
			auto index = platform == kBuildPlatformMacOS ? 0 : 1;
			AddCStringList(rules, kXcodeFrameworkProperties[index]);
			AddKnown<Data::BoolProperty>(rules, kXcodeArcProperties[index]);
			if (platform == kBuildPlatformMacOS) AddKnown<Data::BoolProperty>(rules, kMacOSBuildAllArchitectures);
			AddKnown<Data::CStringProperty>(rules, kXcodeBundleIdentifierProperties[index]);
			AddKnown<Data::BoolProperty>(rules, kXcodeCodesignProperties[index]);
			AddKnown<Data::CStringProperty>(rules, kXcodeDevelopmentTeamProperties[index]);
			AddKnown<Data::CStringProperty>(rules, kXcodeInfoPlistProperties[index]);
			AddKnown<Data::CStringProperty>(rules, kXcodeEntitlementsProperties[index]);
			AddKnown<Data::CStringProperty>(rules, kXcodeIconProperties[index]);
			AddKnown<Data::Int32Property>(rules, kXcodeMinimumVersionProperties[index]);
			AddKnown<Data::Float32Property>(rules, kXcodeMinimumVersionProperties[index]);
			if (platform == kBuildPlatformMacOS) AddKnown<Data::CStringProperty>(rules, kMacOSExportedSymbols);
			else AddKnown<Data::CStringProperty>(rules, kXcodeLaunchScreenProperty);
			break;
		}

		case kBuildPlatformAndroid:
			AddKnown<Data::Int32Property>(rules, kAndroidSdk);
			AddKnown<Data::Float32Property>(rules, kAndroidSdk);
			AddKnown<Data::Int32Property>(rules, kAndroidMinSdk);
			AddKnown<Data::Float32Property>(rules, kAndroidMinSdk);
			AddKnown<Data::CStringProperty>(rules, kAndroidSdkPath);
			AddKnown<Data::CStringProperty>(rules, kAndroidPackageId);
			AddKnown<Data::CStringProperty>(rules, kAndroidArchiveName);
			AddKnown<Data::BoolProperty>(rules, kAndroidNativeAppGlue);
			AddCStringList(rules, kAndroidDependencies);
			AddKnown<Data::CStringProperty>(rules, kAndroidAssets);
			break;

		default:
			break;
		}
	}

	FormatHolder()
	{
		AddFree<Data::CStringProperty>(variables_rules, &HandleVariable);

		AddFree<Data::CStringProperty>(files_rules);
		AddFree<Data::ArrayOfCStringProperty>(files_rules);
		AddFree<Data::PropertySet>(files_rules, &HandleValidateFiles);
		AddFree<Data::Key32Property>(files_rules, &HandleReject);

		AddKnown<Data::CStringProperty>(build_step_rules, kName);
		AddKnown<Data::BoolProperty>(build_step_rules, kAlwaysRun);
		for (auto id : { kCommand, kInputs, kOutputs }) AddCStringList(build_step_rules, id, false);

		AddCStringList(project_rules, kProjectInclude);
		AddKnown<Data::Int32Property>(project_rules, kFormat);
		AddKnown<Data::CStringProperty>(project_rules, kProjectName);
		AddKnown<Data::Key32Property>(project_rules, kDefaultConfiguration);
		AddVariables(project_rules, kNullKey);
		AddCStringList(project_rules, kImport);
		AddKnown<Data::KeyMap>(project_rules, Data::kkeymap);
		AddKnown<Data::PropertySet>(project_rules, Data::kError);
		AddFree<ValidatedPropertySet>(project_rules, &HandleStructuralChild);

		AddCommon(common_rules);
		Append(structural_rules, common_rules);
		AddKnown<Data::Key32Property>(structural_rules, kInherit, &HandleInherit);
		AddKnown<Data::ArrayOfKey32Property>(structural_rules, kInherit, &HandleInherit);
		AddFree<PlatformPropertySet>(structural_rules, &HandleStructuralChild);

		REFLEX_LOOP(platform, kBuildPlatformCMake)
		{
			Append(platform_rules[platform], common_rules);
			AddPlatform(platform_rules[platform], BuildPlatform(platform));
			AddFree<ValidatedPropertySet>(platform_rules[platform], &HandleStructuralChild);
			Append(configuration_rules[platform], common_rules);
			AddPlatform(configuration_rules[platform], BuildPlatform(platform));
		}

		static const TypeID kSupportedTypes[] =
		{
			REFLEX_TYPEID(ValidatedPropertySet),
			REFLEX_TYPEID(PlatformPropertySet),
		};

		g_project_format = Data::Detail::CreatePropertySetFormat(New<PropertySheetInterface>(), kSupportedTypes);
		Retain(g_project_format);
	}

	~FormatHolder()
	{
		Release(g_project_format);
	}

	PropertyRules empty_rules;
	PropertyRules variables_rules;
	PropertyRules files_rules;
	PropertyRules build_step_rules;
	PropertyRules project_rules;
	PropertyRules common_rules;
	PropertyRules structural_rules;
	PropertyRules platform_rules[kBuildPlatformCMake];
	PropertyRules configuration_rules[kBuildPlatformCMake];
};

Reflex::Detail::Module::Member<ReflexCLI::ProjectGen::FormatHolder> g_format_holder(Bootstrap::module);

bool HasRuleID(ArrayView<PropertyRule> rules, Key32 id)
{
	for (auto & rule : rules) if (rule.address.id == id) return true;
	return false;
}

const PropertyRule * FindFreeRule(ArrayView<PropertyRule> rules, TypeID type)
{
	for (auto & rule : rules) if (rule.address.type_id == type) return &rule;
	return nullptr;
}

const PropertyRules & GetPropertySetRules(ReflexCLI::ProjectGen::PropertySetType type, const Reflex::Data::PropertySet * parent)
{
	using namespace ReflexCLI::ProjectGen;
	switch (type)
	{
	case kPropertySetTypeVariables: return g_format_holder->variables_rules;
	case kPropertySetTypeFiles: return g_format_holder->files_rules;
	case kPropertySetTypeBuildStep: return g_format_holder->build_step_rules;
	case kPropertySetTypeProject: return g_format_holder->project_rules;
	case kPropertySetTypeTemplate:
	case kPropertySetTypeTarget:
	case kPropertySetTypeLibrary: return g_format_holder->structural_rules;
	case kPropertySetTypeConfiguration: return g_format_holder->configuration_rules[Cast<PlatformPropertySet>(*parent)->GetPlatform()];
	default: return g_format_holder->empty_rules;
	}
}

template <class ITEM, class ARRAY> Array <ITEM> GetItems(const ValidatedPropertySet & values, Key32 id)
{
	Array<ITEM> result;
	Array<const ValidatedPropertySet *> scopes;
	for (auto scope = &values; scope; scope = DynamicCast<ValidatedPropertySet>(*scope->parent)) scopes.Push(scope);
	for (UInt index = scopes.GetSize(); index--;)
	{
		auto scope = scopes[index];
		if (HasUnset(*scope, id)) result.Clear();
		auto property_object = QueryLocalProperty(RemoveConst(*scope), MakeAddress<AppendingArray>(id));
		if (auto property = property_object ? DynamicCast<AppendingArray>(*property_object) : nullptr)
		{
			REFLEX_ASSERT(property->type == GetTypeID<ARRAY>());
			result.Append(Cast<ARRAY>(*property->values)->value);
			continue;
		}
		if (auto property = QueryLocalProperty(RemoveConst(*scope), MakeAddress<ARRAY>(id)))
		{
			result.Clear();
			result.Append(Cast<ARRAY>(*property)->value);
		}
	}
	return result;
}

REFLEX_END_INTERNAL

void ReflexCLI::ProjectGen::ValidatedPropertySet::StoreProperty(Address address, Object & object)
{
	Data::PropertySet::OnSetProperty(address, object);
}

void ReflexCLI::ProjectGen::ValidatedPropertySet::OnSetProperty(Address address, Object & object)
{
	Address lookup = address;

	if (IsUnset(address))
	{
		RequireStructure(HasRuleID(m_rules.known_properties, address.id) || m_rules.free_properties.GetSize(), object, "unknown property");
		RemovePropertiesWithID(*this, address.id);
		Data::PropertySet::OnSetProperty(address, object);
		return;
	}

	const PropertyRule * rule = SearchValue<FieldCompare<&PropertyRule::address>>(m_rules.known_properties, lookup);
	if (!rule)
	{
		rule = FindFreeRule(m_rules.free_properties, address.type_id);
		RequireStructure(rule || !HasRuleID(m_rules.known_properties, lookup.id), object, "invalid property type");
	}
	RequireStructure(rule, object, "unknown property");

	rule->handler(*this, *rule, address, object);
}

ReflexCLI::ProjectGen::ValidatedPropertySet::ValidatedPropertySet()
	: ValidatedPropertySet(kPropertySetTypeTarget, Null<Data::KeyMap>(), Data::PropertySet::null)
{
}

REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, ValidatedPropertySet);
REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, Unset);
REFLEX_BOOTSTRAP_NULL_INSTANCE(ReflexCLI::ProjectGen, AppendingArray);

ReflexCLI::ProjectGen::ValidatedPropertySet::ValidatedPropertySet(PropertySetType type, const Data::KeyMap & keymap, Data::PropertySet & parent, Key32 name)
	: type(type)
	, keymap(keymap)
	, parent(parent)
	, id(name)
	, m_rules(GetPropertySetRules(type, &parent))
{
}

ReflexCLI::ProjectGen::ValidatedPropertySet::ValidatedPropertySet(PropertySetType type, const PropertyRules & rules, const Data::KeyMap & keymap, Data::PropertySet & parent, Key32 name)
	: type(type)
	, keymap(keymap)
	, parent(parent)
	, id(name)
	, m_rules(rules)
{
}

bool ReflexCLI::ProjectGen::ValidatedPropertySet::IsRoot() const
{
	return parent.Adr() == &Data::PropertySet::null;
}

Reflex::Array<Reflex::CString> ReflexCLI::ProjectGen::ValidatedPropertySet::GetCStrings(Key32 id) const
{
	return GetItems<CString, Data::ArrayOfCStringProperty>(*this, id);
}

Reflex::Array<Reflex::Key32> ReflexCLI::ProjectGen::ValidatedPropertySet::GetKeys(Key32 id) const
{
	return GetItems<Key32, Data::ArrayOfKey32Property>(*this, id);
}

Reflex::Array<Reflex::Reference<Reflex::Data::PropertySet>> ReflexCLI::ProjectGen::ValidatedPropertySet::GetPropertySets(Key32 id) const
{
	return GetItems<Reference<Data::PropertySet>, Data::PropertySetArray>(*this, id);
}

ReflexCLI::ProjectGen::PlatformPropertySet::PlatformPropertySet(const Data::KeyMap & keymap, Data::PropertySet & parent, BuildPlatform platform)
	: ValidatedPropertySet(kPropertySetTypePlatform, g_format_holder->platform_rules[platform], keymap, parent, MakeKey32(kBuildPlatforms[platform]))
	, m_platform(platform)
{
}

void ReflexCLI::ProjectGen::ValidatedPropertySet::OnQueryProperty(Address address, Object * & object) const
{
	auto fallback = object;
	Data::PropertySet::OnQueryProperty(address, object);
	if (object != fallback || type <= kPropertySetTypeBuildStep) return;

	// A value of another type at this scope still masks the inherited value.
	for (auto & item : Iterate()) if (item.key.id == address.id) return;
	object = parent->QueryProperty(address, fallback);
}

ReflexCLI::ProjectGen::Project::Project(const Data::KeyMap & keymap)
	: ValidatedPropertySet(kPropertySetTypeProject, keymap)
{
}

ReflexCLI::BuildPlatform ReflexCLI::ProjectGen::PlatformPropertySet::GetPlatform() const
{
	return m_platform;
}

Reflex::ConstTRef <Reflex::Data::Format> ReflexCLI::ProjectGen::g_project_format = kNoValue;
