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
		ValidatedPropertySet(PropertySetType type, const Data::KeyMap & keymap = Null<Data::KeyMap>(), Data::PropertySet & parent = Data::PropertySet::null, Key32 name = {});

		bool IsRoot() const;
		bool IsEnabled() const { return Data::GetBool(*this, kEnabled, true); }
		Array<CString> GetCStrings(Key32 id) const;
		Array<Key32> GetKeys(Key32 id) const;
		Array<Reference<Data::PropertySet>> GetPropertySets(Key32 id) const;

		const ConstTRef<Data::KeyMap> keymap;
		const TRef<Data::PropertySet> parent;
		const Key32 id;

	protected:
		ValidatedPropertySet(PropertySetType type, const PropertyRules & rules, const Data::KeyMap & keymap, Data::PropertySet & parent, Key32 name = {});
		void OnSetProperty(Address address, Object & object) override;
		void OnQueryProperty(Address address, Object * & object) const override;

		const PropertyRules & m_rules;
	};

	struct PlatformPropertySet : public ValidatedPropertySet
	{
		REFLEX_OBJECT(PlatformPropertySet, ValidatedPropertySet);

		PlatformPropertySet(const Data::KeyMap & keymap, Data::PropertySet & parent, BuildPlatform platform = kBuildPlatformWindows);

		BuildPlatform GetPlatform() const;

	private:
		BuildPlatform m_platform;
	};

	extern ConstTRef<Data::Format> g_project_format;
}
