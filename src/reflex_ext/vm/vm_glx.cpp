#pragma once

#include "../glx/animation/property.h"





//TODO

REFLEX_BEGIN_INTERNAL(Reflex::VM)

REFLEX_DECLARE_KEY32(from);
REFLEX_DECLARE_KEY32(to);

REFLEX_INLINE GLX::InterpolatedAnimation * CreatePropertyAnimationImpl(Data::PropertySet & params)
{
	struct PropertyAnimation : public GLX::AbstractPropertyAnimation
	{
		PropertyAnimation(const GLX::AbstractPropertyAnimation::TypeHandler & type, Key32 property_id)
			: GLX::AbstractPropertyAnimation(type, property_id)
		{
		}

		using GLX::AbstractPropertyAnimation::m_from_to;
	};

	auto property_id = Data::GetKey32(params, K32("id"));

	for (auto & type : PropertyAnimation::kTypeHandlers)
	{
		auto type_id = type.type_id;

		if (auto pto = params.QueryProperty({ kto, type_id }))
		{
			if (auto pfrom = params.QueryProperty({ kfrom, type_id }))
			{
				auto t = Reflex::Detail::Constructor<PropertyAnimation>::CreateVariableSize(g_default_allocator, sizeof(Float32) * type.n, type, property_id);

				auto dst = t->m_from_to;

				REFLEX_LOOP_PTR(PropertyAnimation::GetValueAdr(type, *pfrom), ptr, type.n) *dst++ = *ptr;

				REFLEX_LOOP_PTR(PropertyAnimation::GetValueAdr(type, *pto), ptr, type.n) *dst++ = *ptr;

				return t;
			}
		}
	}

	return &GLX::InterpolatedAnimation::null;
}

const Module g_glx_ext_animation("GLX > Ext > Animation", { GLXVM::gGLX }, kContextFlagUi, [](Compiler::State & state, UInt8 context_flags, Object&)
{
	REFLEX_DECLARE_KEY32(GLX);


	auto bindings = state.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto uint8_t = bindings->uint8_t;

	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;


	auto propertyset_t = VM::GetType<Data::PropertySet>(bindings);

	auto object_t = VM::GetType<GLX::Object>(bindings);

	auto animation_t = VM::GetType<GLX::Animation>(bindings);

	auto interpolated_animation_t = VM::GetType<GLX::InterpolatedAnimation>(bindings);


	auto playlist_t = RegisterObject<GLX::PlayList>(bindings, kGLX, "Sequence");

	VM::Detail::BindObjectMethod<&GLX::PlayList::EnableLoop>(bindings, playlist_t, "EnableLoop");

	RegisterObject<GLX::Multi>(bindings, kGLX, "Multi");



	AddFunction(bindings, kGLX, "CreateAnimation", animation_t, { key32_t, propertyset_t }, [](Context & context)
	{
		VM_POP(UInt32,Data::PropertySet&);

		GLX::Animation * rtn;

		switch (args.a)
		{
		case K32("Property"):
			rtn = CreatePropertyAnimationImpl(args.b);
			break;

		case K32("Opacity"):
			rtn = GLX::CreateOpacityAnimation(Data::GetKey32(args.b, GLX::kid, kNullKey), Data::GetFloat32(args.b, kfrom), Data::GetFloat32(args.b, kto)).Adr();
			break;

		case K32("Wait"):
			rtn = GLX::CreateWaitAnimation().Adr();
			break;

		case K32("Position"):
			rtn = GLX::CreatePositionAnimation(Data::GetBool(args.b, GLX::kaxis), Data::GetFloat32(args.b, kfrom), Data::GetFloat32(args.b, kto)).Adr();
			break;

		case K32("MaxBounds"):
			rtn = GLX::CreateMaxBoundsAnimation(Data::GetKey32(args.b, GLX::kid), Data::GetBool(args.b, GLX::kaxis), Data::GetFloat32(args.b, kfrom), Data::GetFloat32(args.b, kto)).Adr();
			break;

		case K32("State"):
			rtn = GLX::CreateStateAnimation(Data::GetKey32(args.b, K32("state"), GLX::kSelectedState)).Adr();
			break;

		default:
			rtn = &GLX::Animation::null;
			break;
		}

		VM_RTN(rtn);
	});


	AddFunction(bindings, kGLX, "Run", void_t, { object_t, key32_t, animation_t }, [](Context & context)
	{
		VM_POP(GLX::Object&, Key32, GLX::Animation&);

		GLX::Run(args.a, args.b, args.c);
	});

	AddFunction(bindings, kGLX, "Run", void_t, { object_t, key32_t, float32_t, animation_t }, [](Context & context)
	{
		VM_POP(GLX::Object&, Key32, Float32, GLX::Animation&);

		GLX::Run(args.a, args.b, args.c, args.d);
	});

	AddFunction(bindings, kGLX, "Run", void_t, { object_t, key32_t, float32_t, key32_t, interpolated_animation_t }, [](Context & context)
	{
		VM_POP(GLX::Object&, Key32, Float32, Key32, GLX::InterpolatedAnimation&);

		GLX::Run(args.a, args.b, args.c, GLX::Detail::GetEasing(args.d), args.e);
	});


	
	//helpers

	AddFunction(bindings, kGLX, "Enter", void_t, { object_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt8);

		GLX::Enter(args.a, args.b);
	});

	AddFunction(bindings, kGLX, "SkipEnter", void_t, { object_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt8);

		GLX::SkipEnter(args.a, args.b);
	});

	AddFunction(bindings, kGLX, "Exit", void_t, { object_t, bool_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,bool,UInt8);

		GLX::Exit(args.a, args.b, args.c);
	});

	AddFunction(bindings, kGLX, "SkipExit", void_t, { object_t, bool_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,bool, UInt8);

		GLX::SkipExit(args.a, args.b, args.c);
	});
});

REFLEX_END_INTERNAL
