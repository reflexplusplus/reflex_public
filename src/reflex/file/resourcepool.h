#pragma once

#include "../../../include/reflex/file/resourcepool.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::File)

struct ResourcePoolImpl : public ResourcePool
{
	ResourcePoolImpl(VirtualFileSystem & filesystem);

	~ResourcePoolImpl();


	UInt GetTotalNumItem() const override;

	const UInt & GetNumItem(Attributes::Status status) const override;


	bool Flush(UInt cycles);

	void RemoveItem(UInt idx);



	Sequence <Address,Token> m_items;

	UInt m_nitems[3];

	UInt m_flush_position;

	UInt m_flushing;	//debug

};

REFLEX_END_INTERNAL
