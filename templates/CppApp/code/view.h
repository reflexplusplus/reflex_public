#pragma once

#include "app.h"




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

	static Reflex::TRef <View> Create(App & app);



protected:

	using Reflex::Bootstrap::View::View;

};
