#pragma once

#include "instance.h"




//
//declarations

namespace _PRODUCT-NAME-SYMBOL_
{
	
	class View;
	
}




//
//_PRODUCT-NAME_ View

class _PRODUCT-NAME-SYMBOL_::View : public Reflex::Bootstrap::View
{
public:

	static Reflex::Unretained <View> Create(Instance & instance);



protected:

	using Reflex::Bootstrap::View::View;

};
