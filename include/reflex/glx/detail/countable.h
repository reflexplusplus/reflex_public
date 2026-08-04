#pragma once

#include "[require].h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

template <UInt ID> class Countable;

REFLEX_END




//
//font

template <Reflex::UInt ID>
class Reflex::GLX::Detail::Countable
{
public:

	Countable() { st_count++; }

	~Countable() { st_count--; }

	static const UInt & GetCount() { return st_count;  }



private:

	static UInt st_count;
};




//
//

template <Reflex::UInt ID> inline Reflex::UInt Reflex::GLX::Detail::Countable<ID>::st_count = 0;
