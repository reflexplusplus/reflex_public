#pragma once

#include "../defines.h"




REFLEX_NS(Reflex::Data)

void LineBreak(Data::Archive & archive);

template <class TYPE> inline bool CheckObjectArrayType(const Array <Detail::StandardPropertySheetInterface::ObjectWithType> & objects)
{
	for (auto & i : objects)
	{
		if (!DynamicCast<TYPE>(i.a)) return false;
	}

	return true;
}

struct PropertySheetFormatImpl : public Format
{
	struct ExportState : public Object
	{
		ExportState(const PropertySet & root)
			: keymap(GetKeyMap(root))
		{
		}

		const KeyMap & keymap;
		CString indent;
	};

	PropertySheetFormatImpl(Detail::PropertySheetInterface & iface);

	void RegisterType(TypeID type_id) { m_types[type_id]; }

	template <class TYPE> TypeID RegisterType() { RegisterType(REFLEX_TYPEID(TYPE)); return REFLEX_TYPEID(TYPE); }


	virtual TRef <ExportState> PrepareExport(const PropertySet & root) const { return REFLEX_CREATE(ExportState, root); }

	virtual bool Export(const PropertySet & cpropertyset, ExportState & state, Archive & out) const = 0;


	bool SupportsType(TypeID type_id) const override;

	void OnReset(PropertySet & data) const override;

	bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const override;

	bool OnEncode(Archive & out, const PropertySet & in, UInt32 options) const override;

	void WriteNode(const PropertySet & propertyset, ExportState & state, Archive & archive, char delim) const;


	Detail::PropertySheetInterface & iface;

	Map <TypeID> m_types;

	Reference <Object> m_clientdata;
};

REFLEX_END
