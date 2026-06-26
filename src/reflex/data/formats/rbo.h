#pragma once

#include "../../../../include/reflex/data/format/formats/detail/propertyset_format.h"




REFLEX_NS(Reflex::Data)

struct PropertySetFormatImpl : public Detail::PropertySetFormat
{
	PropertySetFormatImpl(UInt32 magic);

	TRef <PropertySetFormat> Clone(UInt32 magic) const override
	{
		auto f = REFLEX_CREATE(PropertySetFormatImpl, *this);

		f->m_magic = magic;

		return f;
	}

	void SetTypeHandler(const TypeHandler & type_handler) override
	{
		REFLEX_ASSERT(!m_persistentids.Search(type_handler.persistentid) && !m_types.Search(type_handler.type));

		REFLEX_ASSERT(type_handler.store && type_handler.restore && type_handler.compare && type_handler.null);

		m_persistentids[type_handler.persistentid] = &m_types.Acquire(type_handler.type, type_handler);
	}

	const TypeHandler * QueryTypeHandler(TypeID type) const override
	{
		return m_types.Search(type);
	}

	bool SupportsType(TypeID type_id) const override;

	Array <Address> Compare(const PropertySet & a, const PropertySet & b) const override;

	
	void OnReset(PropertySet & data) const override;

	DeserializeError OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const override;

	void OnSerialize(Archive & stream, const PropertySet & data) const override;


	void DeserializePropertySetImpl(Archive::View & stream, PropertySet & data) const;	//throws DeserializeError

	void SerializePropertySetImpl(Archive & stream, const PropertySet & data) const;


	static bool CompareDynamic(const PropertySetFormat & format, const PropertySet & a, const PropertySet & b);

	static bool CompareArraysOfDynamics(const PropertySetFormat & format, const PropertySetArray & a, const PropertySetArray & b);



	UInt32 m_magic;

	Map <TypeID,TypeHandler> m_types;

	Map <UInt8,TypeHandler*> m_persistentids;
};

struct BinaryFormat : public SerializableFormat
{
	static constexpr UInt32 kMagicV3 = 730859756;
	static constexpr UInt32 kLegacyV2 = 0x1ee4737f;
	static constexpr UInt64 kLegacyV1 = 3472651841318515280ull;


	bool SupportsType(TypeID tid) const override;

	void OnReset(PropertySet & node) const override;

	DeserializeError OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const override;

	void OnSerialize(Archive & stream, const PropertySet & data) const override;

	bool ImportLegacy(UInt version, bool _64bit, PropertySet & registry, Archive::View & stream) const;
};

void InitialiseStandardPropertySet(PropertySetFormatImpl & format);

REFLEX_END
