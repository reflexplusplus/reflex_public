#include "reflex/data/format/functions/keymap.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

constexpr Pair <Key32,CString::View> kKeymapDefaults[] =
{
	{ K32(""), "" },

	{ K32("true"), "true" },
	{ K32("false"), "false" },

	{ K32("keymap"), "keymap" },
	{ kid, "id" },

	{ K32("type"), "type" },		//what for?

	{ K32("index"), "index" },

	{ K32("tag"), "tag" },		//xml
	{ K32("nodes"), "nodes" }	//xml
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Data::KeyMap> Reflex::Data::AcquireKeyMap(PropertySet & node)
{
	auto keymap = Detail::AcquireProperty<KeyMap>(node, kkeymap);

	for (auto & i : kKeymapDefaults) keymap->value.Set(i.a, i.b);

	return keymap;
}

Reflex::Key32 Reflex::Data::RegisterKey(KeyMap & keymap, const CString::View & string)
{
	Key32 key = string;

	if constexpr (REFLEX_DEBUG)
	{
		if (auto current = keymap.value.Search(key))
		{
			if (*current != string) File::output.Error("Data::RegisterKey collision");
		}
	}

	keymap.value.Set(key, string);

	return key;
}

void Reflex::Data::Assimilate(KeyMap & keymap, const KeyMap & b)
{
	for (auto & i : b.value)
	{
		keymap.value.Set(i.key, i.value);
	}
}

Reflex::ConstTRef <Reflex::Data::KeyMap> Reflex::Data::GetKeyMap(const PropertySet & root)
{
	return GetProperty<KeyMap>(root, kkeymap);
}

Reflex::CString::View Reflex::Data::GetKey(const KeyMap & keymap, Key32 key) 
{ 
	return *keymap.value.Search(key, &Null<CStringProperty>()->value);
}
