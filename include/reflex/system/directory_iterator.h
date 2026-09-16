#pragma once

#include "types.h"




//
//Secondary API

namespace Reflex::System
{

	class DirectoryIterator;

}




//
//DirectoryIterator

class Reflex::System::DirectoryIterator : public Object
{
public:

	REFLEX_OBJECT(System::DirectoryIterator, Object);

	static DirectoryIterator & null;

	struct Item
	{
		WString filename;
		bool is_directory;
	};



	//lifetime

	[[nodiscard]] static Unretained <DirectoryIterator> Create(const WString & directory, bool hidden);



	//interface

	virtual bool GetNext(Item & item) = 0;

};
