#pragma once

#include "tasks.h"

namespace ReflexCLI::ProjectGen
{
	constexpr CString::View kInherit = "inherit";
	constexpr CString::View kEnabled = "enabled";

	enum PropertySetType : UInt8
	{
		kPropertySetTypeVariables,
		kPropertySetTypeFiles,
		kPropertySetTypeBuildStep,
		kPropertySetTypeProject,
		kPropertySetTypeTemplate,
		kPropertySetTypeTarget,
		kPropertySetTypeLibrary,
		kPropertySetTypePackage,
		kPropertySetTypePlatform,
		kPropertySetTypeConfiguration,
	};

	struct PropertyRules;

	struct ValidatedPropertySet : public Data::PropertySet
	{
		REFLEX_OBJECT(ValidatedPropertySet, Data::PropertySet);
		static ValidatedPropertySet & null;
		void StoreProperty(Address address, Object & object);
		const PropertySetType type;
		ValidatedPropertySet();
		ValidatedPropertySet(PropertySetType type, const Data::KeyMap & keymap = Null<Data::KeyMap>(), ValidatedPropertySet & parent = ValidatedPropertySet::null, Key32 name = {});

		bool IsRoot() const;
		bool IsEnabled() const { return Data::GetBool(*this, kEnabled, true); }

		const ConstTRef<Data::KeyMap> keymap;
		const TRef<ValidatedPropertySet> parent;
		const Key32 id;

	protected:
		ValidatedPropertySet(PropertySetType type, const PropertyRules & rules, const Data::KeyMap & keymap, ValidatedPropertySet & parent, Key32 name = {});
		void OnSetProperty(Address address, Object & object) override;
		void OnQueryProperty(Address address, Object * & object) const override;

		const PropertyRules & m_rules;
	};

	struct PlatformPropertySet : public ValidatedPropertySet
	{
		REFLEX_OBJECT(PlatformPropertySet, ValidatedPropertySet);

		PlatformPropertySet(const Data::KeyMap & keymap, ValidatedPropertySet & parent, BuildPlatform platform = kBuildPlatformWindows);

		BuildPlatform GetPlatform() const;

	private:
		BuildPlatform m_platform;
	};


	template <class TYPE>
	struct AppendableList : public Object
	{
		using ItemType = TYPE;
		using ValueArray = ConditionalType <kIsObject<TYPE>,Array <Reference<TYPE>>,Array<TYPE>>;
		ValueArray values;
		UInt local_offset = 0;
	};

	struct AppendableStrings : public AppendableList <CString>
	{
		REFLEX_OBJECT(AppendableStrings, Object);
		static AppendableStrings & null;
	};

	struct AppendableKeys : public AppendableList <Key32>
	{
		REFLEX_OBJECT(AppendableKeys, Object);
		static AppendableKeys & null;
	};

	struct AppendablePropertySet : public AppendableList <Data::PropertySet>
	{
		REFLEX_OBJECT(AppendablePropertySet, Object);
		static AppendablePropertySet & null;
	};


	extern ConstTRef<Data::Format> g_project_format;
}
