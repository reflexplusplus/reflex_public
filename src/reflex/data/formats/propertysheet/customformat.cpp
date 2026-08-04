#include "formatimpl.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct CustomFormat : public PropertySheetFormatImpl
{
	CustomFormat(Detail::PropertySheetInterface & iface, const ArrayView <TypeID> & supported)
		: PropertySheetFormatImpl(iface)
	{
		for (auto & i : supported) RegisterType(i);
	}

	bool Export(const PropertySet & cpropertyset, ExportState & state, Archive & out) const override
	{
		return false;
	}
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Data::Format> Reflex::Data::Detail::CreateCustomFormat(PropertySheetInterface & callbacks, const ArrayView <TypeID> & supported)
{
	return REFLEX_CREATE(CustomFormat, callbacks, supported);
}
