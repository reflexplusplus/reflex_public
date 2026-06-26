#include "reflex/glx/event.h"




//
//

Reflex::GLX::Event::List Reflex::GLX::Event::st_list;

Reflex::GLX::Event::Event(Key32 id, Data::PropertySet && params)
	: Item<Event, false, Data::PropertySet>::Item(std::move(params))
	, id(id)
{
}

