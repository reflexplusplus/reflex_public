#include "interface.h"




//
//state

Int32 gCount;




//
//Bootstrap persistence callbacks

void OnReset()
{
	gCount = 0;
}

void OnRestore(Data::BinaryObject chunk)
{
	Array@UInt8 stream = chunk;
	
	Data::Deserialize(stream, gCount);
}

Data::BinaryObject OnStore()
{
	Array@UInt8 stream;
	
	Data::Serialize(stream, gCount);
	
	return stream;
}




//
//implement and publish interface

self#iface = new Interface
{
	.ResetCount = []()
	{
		gCount = 0;

		self.Notify(true);
	},

	.IncCount = [](Int32 inc)
	{
		gCount = gCount + inc;
		
		self.Notify(true);
	},

	.GetCount = []Int32()
	{
		return gCount;
	}
};

