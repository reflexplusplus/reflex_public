#include "../propertyeditorimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE::Detail)

struct XmlInterface : public PropertySheetInterface
{
	using PropertySheetInterface::PropertySheetInterface;

	void SetChildren(Data::PropertySet & node, Children children) const override
	{
		Data::UnsetAll<Data::PropertySet>(node);

		for (auto & i : children)
		{
			auto child = Data::AddXmlNode(i.c, i.a);

			SetProperty(child, Data::kid, i.b);
		}
	}

	Children GetChildren(Data::PropertySet & node) const override
	{
		Children rtn;

		rtn.Allocate(8);

		for (auto & i : node.Iterate<Data::PropertySet>())
		{
			auto tag = Data::GetXmlTag(i.value);

			auto id = Data::GetXmlAttribute(i.value, Data::kid);

			rtn.Push({ tag, id, i.value });
		}

		return rtn;
	}

	virtual Attributes GetAttributes(Data::PropertySet & node) const override
	{
		Attributes attributes;

		for (auto & i : node.Iterate<Data::CStringProperty>())
		{
			auto & key = *keymap.Search(i.key.id, &Reflex::Detail::GetNullInstance(Data::CStringProperty));

			REFLEX_ASSERT(key);

			attributes.Push({kTypeCString, key, Data::Pack<CString>(**i)});
		}

		return attributes;
	};
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::IDE::Detail::PropertyEditor::Interface> Reflex::IDE::Detail::PropertyEditor::CreateXmlInterface(Data::PropertySet & root)
{
	return REFLEX_CREATE(XmlInterface, root);
}
