#include "library.h"




//
//

const Reflex::VM::Type::CrossContextCopyFn Reflex::VM::Detail::kNoContextCopy = [](Context &, Object &, TypeRef, bool) -> Reflex::Object *
{
	return 0;
};

const Reflex::VM::Type::CrossContextCopyFn Reflex::VM::Detail::kTrivialContextCopy = [](Context & context, Object & object, TypeRef to, bool) -> Reflex::Object *
{
	return &object;
};

Reflex::Object * Reflex::VM::Detail::CrossContextCopy(Object & object, Context & to, bool mt, Object * fallback)
{
	REFLEX_ASSERT(!object.DataReleased());

	bool visible_to_context = false;

	if (auto to_t = to.program->bindings->GetTypeByRTTID(object.object_t->type_id))
	{
		visible_to_context = true;

		if (auto copy = to_t->contextcopyfn[mt](to, object, to_t, mt)) return copy;
	}


	//look for final base class, then its also ok to copy using that copier

	auto baseitr = object.object_t->base;

	while (baseitr)
	{
		if (auto base_t = to.program->bindings->GetTypeByRTTID(baseitr->type_id))
		{
			if (base_t->flags.Check(Type::kFlagFinal))
			{
				visible_to_context = true;

				if (auto copy = base_t->contextcopyfn[mt](to, object, base_t, mt)) return copy;
			}
		}

		baseitr = baseitr->base;
	}


	//fail

	if (visible_to_context)
	{
		LogInstruction(kLogWarning, to, Join(object.object_t->tname, " x-context copy fail"));
	}

	return fallback;
}
