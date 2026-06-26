#pragma once

VM_WRAP_INTERNAL(GLX)

template <class TYPE> void BindIterable(VM::Compiler::State & compilestate, Key32 ns, VM::TypeRef object_t)
{
	REFLEX_USE(VM::Detail);

	VM::Detail::BindIterable<TYPE>(compilestate, ns, object_t);

	struct Local
	{
		REFLEX_INLINE static void SetParentImpl(GLX::Object & object, GLX::Object & parent)
		{
			object.SetParent(parent);
		}

		REFLEX_INLINE static void SetParentImpl(GLX::Style & style, GLX::Style & parent)
		{
			style.Attach(parent);
		}
	};

	auto bindings = compilestate.bindings;

	auto void_t = bindings->void_t;

	AddMethod(bindings, "SetParent", void_t, { object_t, object_t }, [](VM::Context & context)
	{
		VM_POP(TYPE&,TYPE&);	//NEEDS SAFE because can trigger callbacks

		auto & self = args.a;
		auto & parent = args.b;

		if (Reflex::BranchContains(self, parent)) parent.Detach();

		Local::SetParentImpl(self, parent);
	});

	AddMethod(bindings, "InsertBefore", void_t, { object_t, object_t }, [](VM::Context & context)
	{
		VM_POP(TYPE&,TYPE&);

		auto & self = args.a;
		auto & sibling = args.b;

		if (&self != &sibling)
		{
			self.InsertBefore(sibling);
		}
	});

	AddMethod(bindings, "InsertAfter", void_t, { object_t, object_t }, [](VM::Context & context)
	{
		VM_POP(TYPE&,TYPE&);

		auto & self = args.a;
		auto & sibling = args.b;

		if (&self != &sibling)
		{
			self.InsertAfter(sibling);
		}
	});

	AddMethod(bindings, "Clear", void_t, { object_t }, [](VM::Context & context)
	{
		Pop<TYPE&>(context.stack).Clear();
	});

	AddMethod(bindings, "Detach", void_t, { object_t }, [](VM::Context & context)
	{
		Pop<TYPE&>(context.stack).Detach();
	});

	//AddFunction(bindings, object_t->symbol.a, VM::Compiler::opCast, bindings.bool_t, { object_t }, { bindings.bool_t }, {}, [](VM::Context & context)
	//{
	//	VM_RTN(Pop<TYPE*>(context.stack) != &REFLEX_NULL(TYPE));
	//});
}

REFLEX_END_INTERNAL
