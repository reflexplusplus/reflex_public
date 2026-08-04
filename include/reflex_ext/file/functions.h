#pragma once

#include "monolith.h"




//
//Secondary API

namespace Reflex::File
{

	void WritePartition(Monolith & monolith, Key64 id, const Data::Archive::View & view);

	Data::Archive ReadPartition(const Monolith & monolith, Key64 id);


	TRef <System::FileHandle> CreateRegionReader(System::FileHandle & file, UInt start, UInt length);

}




//
//impl

inline void Reflex::File::WritePartition(Monolith & monolith, Key64 id, const Data::Archive::View & view)
{
	auto partition = AutoRelease(monolith.Write(id, view.size));

	WriteBytes(partition, view);
}

inline Reflex::Data::Archive Reflex::File::ReadPartition(const Monolith & monolith, Key64 id)
{
	auto partition = AutoRelease(monolith.Read(id));

	return ReadBytes(partition);
}
