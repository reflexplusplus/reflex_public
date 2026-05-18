#pragma once

#include "../[require].h"




//
//

REFLEX_NS(Reflex::VM::Detail)

struct Circular
{
	Circular(Context & context, TRef <Object> object);

	struct TrackingToken : public Reflex::Item <TrackingToken,false>
	{
		TrackingToken(Circular & circular) : circular(circular) {}

		using Item::Attach;

		using Item::Detach;

		Circular & circular;
	};

	Object & object;

	TrackingToken trackingtoken;
};

struct Dummy
{
	template <class ... VARGS> constexpr Dummy(VARGS &&... v) {}
};

REFLEX_END
