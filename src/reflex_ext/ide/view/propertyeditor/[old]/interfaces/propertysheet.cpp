#include "../propertyeditorimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE::Detail)

struct PropertySheetInterface : public PropertyEditor::Interface
{
	PropertySheetInterface(Data::PropertySet & root)
		: root(root),
		keymap(Data::PropertySheet::AquireKeyMap(root))
	{
	}

	virtual Data::PropertySet & GetNull() const override { return Data::PropertySet::null; }

	virtual UInt8 GetFlags() const override { return Interface::kFlagProperties | Interface::kFlagWritable; }

	virtual Reflex::Object & CreateListener(Data::PropertySet & abstractnode, const Function <void(Notification)> & callback) const override
	{
		if (auto xml = DynamicCast<File::PropertySet>(abstractnode))
		{
			return *REFLEX_CREATE(File::PropertySet::Listener, *xml, [callback](Enum notification)
			{
				callback(Notification(notification));
			});
		}

		return Reflex::Object::null;
	}

	Data::PropertySet * CreateNode() const override
	{
		return REFLEX_CREATE(Data::PropertySet);
	}

	void SetChildren(Data::PropertySet & node, Children children) const override
	{
		Data::UnsetAll<Data::PropertySet>(node);

		for (auto & i : children)
		{
			if (auto & type = i.a)
			{
			}
			else
			{
			}

			Reflex::SetProperty(node, i.b, *i.c);
		}
	}

	Children GetChildren(Data::PropertySet & node) const override
	{
		Children rtn;

		rtn.Allocate(8);

		for (auto & i : node.Iterate<Data::PropertySet>())
		{
			auto type = Reflex::GetProperty<CString>(i.value, K32("type"));

			auto & id = Data::PropertySheet::GetKey(keymap, i.key.id);

			rtn.Push({ type, id, i.value });
		}

		return rtn;
	}

	virtual void RemoveAttribute(Data::PropertySet & node, Type type, const CString::View & key) const override
	{
		UnsetProperty<Data::PropertySheet::String>(node, key);
	}

	virtual void SetAttribute(Data::PropertySet & node, Type type, const CString::View & key, const Data::Archive::View & value) const override
	{
		Data::PropertySheet::SetString(keymap, node, key, Data::Unpack<CString::View>(value));
	}

	virtual Attributes GetAttributes(Data::PropertySet & node) const override
	{
		Attributes attributes;

		if (auto pkeymap = Reflex::QueryProperty<Data::KeyMap>(node, Data::PropertySheet::kkeymap))
		{
			for (auto & i : node.Iterate<Data::PropertySheet::String>())
			{
				auto & id = Data::PropertySheet::GetKey(*pkeymap, i.key.id);

				REFLEX_ASSERT(id);

				attributes.Push({ kTypeCString, id, Data::Pack<CString>(*i.value) });
			}
		}
		else
		{
			for (auto & i : node.Iterate<Data::PropertySheet::String>())
			{
				attributes.Push({ kTypeCString, Join('$', Data::BytesToHex(Data::Pack(i.key.id))), Data::Pack<CString>(*i.value)});
			}
		}

		return attributes;
	};

	Reference <Data::PropertySet> root;

	Data::KeyMap & keymap;
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::IDE::Detail::PropertyEditor::Interface> Reflex::IDE::Detail::PropertyEditor::CreatePropertySheetInterface(Data::PropertySet & root)
{
	return REFLEX_CREATE(PropertySheetInterface, root);
}
