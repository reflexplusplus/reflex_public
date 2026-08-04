#include "interface.h"




//
//state

//put state variables here




//
//Bootstrap persistence callbacks

void OnReset()
{
}

void OnRestore(Data::BinaryObject chunk)
{
}

Data::BinaryObject OnStore()
{
	return {};
}




//
//implement and publish interface

self#iface = new Interface
{
	// .Setter = [](Int32 inc)
	// {
		// self.Notify(true);
	// },

	// .Getter = []Int32()
	// {
		// return 0;
	// }
};

