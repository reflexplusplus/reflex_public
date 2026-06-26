#pragma once

#include "iterable.h"





//TODO

#define VM_PUBLISH_OBJECT_METHOD(CLASS,name) constexpr auto k##name##storage = &CLASS::name; const UIntNative k##name = ToUIntNative(&k##name##storage); 

REFLEX_NS(Reflex::GLXVM)

VM_PUBLISH_OBJECT_METHOD(GLX::Object,SetMouseCursor);
VM_PUBLISH_OBJECT_METHOD(GLX::Object,SetState);
VM_PUBLISH_OBJECT_METHOD(GLX::Object,ClearState);
VM_PUBLISH_OBJECT_METHOD(GLX::Object,CheckState);

struct Object::Callbacks : public Reflex::Object
{
	Callbacks(VM::Context & context)
		: context(context),
		onclock(0),
		onalign(0)
	{
	}

	const TRef <VM::Context> context;

	VM::Detail::FnObject * onclock, * onalign;
};

struct AbstractDelegateWrapper : public GLX::Object::Delegate
{
	AbstractDelegateWrapper(GLX::Object & target, UInt32 id, VM::Context & context, VM::Detail::FnObject & fn)
		: context(context),
		fn(fn)
	{
		target.SetDelegate(id, this);
	}

	VM::Context & context;

	Reference <VM::Detail::FnObject> fn;
};

struct ObjectX : public Object
{
	struct LayoutEx : public GLX::Detail::StandardLayout
	{
		LayoutEx(AlignFn align_override)
			: align_override(align_override)
		{
		}

		Pair <AccommodateFn, AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
		{
			auto base = StandardLayout::OnRebuild(object, layout_flags);

			std_align = base.b;

			return { base.a, align_override };
		}

		const AlignFn align_override;

		AlignFn std_align;
	};


	REFLEX_NOINLINE static TRef <Callbacks> GetExt(GLX::Object & object, VM::Context & context)
	{
		return Data::Detail::AcquireProperty<Callbacks>(object, kExtension, context);
	}

	static void OnAlignOverride(GLX::Object & object, bool isresponsive, Float & contenth)
	{
		auto self = Cast<Object>(object);

		auto layout = Cast<LayoutEx>(object.GetLayoutModel());

		REFLEX_ASSERT(self->m_callbacks);

		auto & callbacks = *self->m_callbacks;

		REFLEX_ASSERT(callbacks.onalign);

		//auto standardalign = callbacks.standardalign;			//was needed because could be cleared during call, however, its not possible to trigger

		self->context->Call(*callbacks.onalign, object.GetRect().size);

		layout->std_align(object, isresponsive, contenth);
	}

	static void OnAlignOverrideExternal(GLX::Object & object, bool isresponsive, Float & contenth)
	{
		auto layout = Cast<LayoutEx>(object.GetLayoutModel());

		if (auto callbacks = object.QueryProperty<Callbacks>(kExtension))
		{
			REFLEX_ASSERT(callbacks->onalign);

			//auto standardalign = callbacks->standardalign;			//was needed because could be cleared during call, however, its not possible to trigger

			callbacks->context->Call(*callbacks->onalign, object.GetRect().size);

			layout->std_align(object, isresponsive, contenth);
		}
	}

	using GLX::Object::EnableOnClock;

	using GLX::Object::EnableOnAttachDetachWindow;

	using GLX::Object::SetLayoutModel;
};

VM::TypeRef BindObject(VM::Compiler::State & cstate, Key32 ns, VM::TypeRef point_t, VM::TypeRef size_t, VM::TypeRef rect_t, VM::TypeRef style_t)
{
	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto uint8_t = bindings->uint8_t;

	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;

	auto callback_t = bindings->callback_t;


	auto object_t = VM::RegisterObject<GLX::Object>(bindings, ns, "Object");

	object_t->members.Push(VM_BIND_MEMBER(GLX::Object, id, cstate, key32_t));

	VM::Detail::SetTypeCtr(object_t, {}, [](VM_CTR_PARAMS) -> Reflex::Object &
	{
		return *REFLEX_CREATE(Object, context);
	});

	GLXVM::BindIterable<GLX::Object>(cstate, ns, object_t);

	VM::Detail::BindObjectMethod<&GLX::Object::SendBottom>(bindings, object_t, "SendBottom");

	VM::Detail::BindObjectMethod<&GLX::Object::SendTop>(bindings, object_t, "SendTop");


	auto onclock_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, float32_t });

	auto onstyle_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, style_t });

	auto onrefresh_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, size_t });

	cstate.InstantiateTemplateType(VM::kFn, { bindings->object_t, object_t, bindings->object_t });


	cstate.RegisterConstant(key32_t, ns, "kSelectedState", Data::Pack(GLX::kSelectedState));

	cstate.RegisterConstant(key32_t, ns, "kFocusedState", Data::Pack(GLX::kFocusedState));

	cstate.RegisterConstant(key32_t, ns, "kInactiveState", Data::Pack(GLX::kInactiveState));

	cstate.RegisterConstant(key32_t, ns, "kActiveState", Data::Pack(GLX::kActiveState));



	AddMethod(bindings, "SetLayoutModel", void_t, { object_t, key32_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,Key32);

		auto model = GLX::kStandardLayout;

		switch (args.b.value)
		{
		case K32("standard_wrapped"):
			model = GLX::kStandardLayoutWrapped;
			break;
		}

		args.a.SetLayoutModel(model);
	});


	AddMethod(bindings, "Attached", bool_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(Object&);
		VM_RTN(True(arg.GetWindow()));
	});

	AddMethod(bindings, "SetOnAttachDetach", void_t, { object_t, callback_t, callback_t }, [](VM::Context & context)
	{
		VM_POP(ObjectX&,VM::Detail::FnObject&,VM::Detail::FnObject&);

		auto & self = args.a;

		struct Delegate : public AbstractDelegateWrapper
		{
			Delegate(GLX::Object & target, VM::Context & context, VM::Detail::FnObject & onattach, VM::Detail::FnObject & ondetach)
				: AbstractDelegateWrapper(target, K32("VM:OnAttachDetach"), context, onattach),
				ondetach(ondetach)
			{
			}

			virtual void OnAttachWindow() override { VM::CallReturningVoid(context, fn); }

			virtual void OnDetachWindow() override { VM::CallReturningVoid(context, ondetach); }

			Reference <VM::Detail::FnObject> ondetach;
		};

		New<Delegate>(self, context, args.b, args.c);

		self.EnableOnAttachDetachWindow(true);
	});

	AddMethod(bindings, "ClearOnAttachDetach", void_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(ObjectX&);

		arg.EnableOnAttachDetachWindow(false);

		arg.ClearDelegate(K32("VM:OnAttachDetach"));
	});

	AddMethod(bindings, "SetOnUpdate", void_t, { object_t, callback_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,VM::Detail::FnObject&);

		struct Delegate : public AbstractDelegateWrapper
		{
			using AbstractDelegateWrapper::AbstractDelegateWrapper;

			virtual void OnUpdate() override { VM::CallReturningVoid(context, fn); }
		};

		New<Delegate>(args.a, K32("VM:OnUpdate"), context, args.b);
	});

	AddMethod(bindings, "ClearOnUpdate", void_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(GLX::Object&);

		arg.ClearDelegate(K32("VM:OnUpdate"));
	});

	AddMethod(bindings, "SetOnStyle", void_t, { object_t, onstyle_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,VM::Detail::FnObject&);

		struct Delegate : public AbstractDelegateWrapper
		{
			using AbstractDelegateWrapper::AbstractDelegateWrapper;

			virtual void OnSetStyle(const GLX::Style & style) override 
			{ 
				VM::CallReturningVoid(context, fn, style);
			}
		};

		New<Delegate>(args.a, K32("VM:OnSetStyle"), context, args.b);
	});

	AddMethod(bindings, "ClearOnStyle", void_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(ObjectX&);

		arg.ClearDelegate(K32("VM:OnSetStyle"));
	});

	AddMethod(bindings, "SetOnClock", void_t, { object_t, onclock_t }, [](VM::Context & context)
	{
		VM_POP(ObjectX&,VM::Detail::FnObject*);

		auto ext = ObjectX::GetExt(args.a, context);

		ext->onclock = args.b;

		args.a.SetProperty(K32("OnClock"), args.b);

		args.a.EnableOnClock(true);
	});

	AddMethod(bindings, "ClearOnClock", void_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(ObjectX&);

		if (auto ext = arg.QueryProperty<Object::Callbacks>(Object::kExtension))
		{
			ext->onclock = 0;
		}

		arg.UnsetProperty<VM::Detail::FnObject>(K32("OnClock"));

		arg.EnableOnClock(false);
	});

	AddMethod(bindings, "SetOnRefresh", void_t, { object_t, onrefresh_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,VM::Detail::FnObject*);

		auto & self = args.a;

		auto ext = ObjectX::GetExt(self, context);

		ext->onalign = args.b;
		
		self.SetProperty(K32("OnAlign"), args.b);

		//TODO extend 'standard' data with VM specifics -> can then unify ext and internal and remove checks

		if (auto vmobject = DynamicCast<GLXVM::Object>(self))
		{
			self.SetLayoutModel(New<ObjectX::LayoutEx>(&ObjectX::OnAlignOverride));
		}
		else
		{
			self.SetLayoutModel(New<ObjectX::LayoutEx>(&ObjectX::OnAlignOverrideExternal));
		}
	});

	AddMethod(bindings, "ClearOnRefresh", void_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(GLX::Object&);

		arg.SetLayoutModel(GLX::kStandardLayout);	//CHANGED, untested
	});


	AddMethod(bindings, "GetParent", object_t, { object_t }, [](VM::Context & context)
	{
		VM_POP1(GLX::Object&);

		auto parent = DynamicCast<GLXVM::Object>(arg.GetParent(), Cast<GLXVM::Object>(&GLX::Object::null));

		VM_RTN(parent);
	});


	AddMethod(bindings, "SetFlow", void_t, { object_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt8);

		RemoveConst(args.a.GetLayoutFlags()) = args.b | (args.a.GetLayoutFlags() & Bits<false,false,false,false,true,true,true,true>::value);

		args.a.RebuildLayout();
	});

	AddMethod(bindings, "SetPositioningFlags", void_t, { object_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt8);

		args.a.SetPositioningFlags(args.b);
	});

	AddMethod(bindings, "GetPositioningFlags", uint8_t, { object_t }, [](VM::Context & context)
	{
		VM_RTN(VM::Detail::Pop<GLX::Object&>(context.stack).GetPositioningFlags());
	});


	AddMethod(bindings, "EnableAutoFit", void_t, { object_t, bool_t, bool_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,bool,bool);

		GLX::EnableAutoFit(args.a, args.b, args.c);
	});

	AddFunction(bindings, ns, "SetClip", void_t, { object_t, key32_t, bool_t, bool_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,Key32,bool,bool);

		GLX::SetClip(args.a, args.b, args.c, args.d);
	});

	AddMethod(bindings, "EnableMouse", void_t, { object_t, bool_t, bool_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,bool,bool);

		GLX::EnableMouse(args.a, args.b, args.c);
	});

	bindings->RegisterIntrinsic(ns, "SetMouseCursor", VM::kintrinsicObjectMethod_Void_Value8, kSetMouseCursor, 0, void_t, { object_t, uint8_t }, {}, VM::kMemberFunction);


	AddMethod(bindings, "SetPosition", void_t, { object_t, point_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,GLX::Point);

		args.a.SetPosition(args.b);
	});

	AddMethod(bindings, "GetRect", rect_t, { object_t }, [](VM::Context & context)
	{
		VM_RTN(VM::Detail::Pop<GLX::Object&>(context.stack).GetRect());
	});


	VM::Detail::BindObjectMethod<&GLX::Object::SetStyle>(bindings, object_t, "SetStyle");

	VM::Detail::BindObjectMethod<&GLX::Object::GetStyle>(bindings, object_t, "GetStyle");




	bindings->RegisterIntrinsic(ns, "ClearState", VM::kintrinsicObjectMethod_Void_Value32, kClearState, 0, void_t, { object_t, key32_t }, {}, VM::kMemberFunction);

	bindings->RegisterIntrinsic(ns, "SetState", VM::kintrinsicObjectMethod_Void_Value32, kSetState, 0, void_t, {object_t, key32_t }, {}, VM::kMemberFunction);

	bindings->RegisterIntrinsic(ns, "CheckState", VM::kintrinsicObjectMethod_Value8_Value32, kCheckState, 0, bool_t, {object_t, key32_t }, {}, VM::kMemberFunction);


	VM::Detail::BindObjectMethod<&GLX::Object::Focus>(bindings, object_t, "Focus");
	VM::Detail::BindObjectMethod<&GLX::Object::Update>(bindings, object_t, "Update");
	VM::Detail::BindObjectMethod<&GLX::Object::Realign>(bindings, object_t, "Refresh");
	//VM::Detail::BindObjectMethod<&GLX::Object::Accommodate>(bindings, object_t, "Accommodate");
	VM::Detail::BindObjectMethod<&GLX::Object::RebuildLayout>(bindings, object_t, "RebuildLayout");

	VM::AddFunction(bindings, ns, "SetState", void_t, { object_t, key32_t, bool_t }, [](VM::Context & context)
	{
		VM_POP(Object&,Key32,bool);

		if (args.c)
		{
			args.a.SetState(args.b);
		}
		else
		{
			args.a.ClearState(args.b);
		}
	});

	AddFunction(bindings, ns, "CalculateRelativeRect", rect_t, { object_t, object_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,GLX::Object&);

		VM_RTN(CalculateRelativeRect(args.a, args.b));
	});


	return object_t;
}

REFLEX_END
