#pragma once

#include "../../../glx/style/transition.h"





//TODO

REFLEX_NS(Reflex::GLXVM)

struct AnimationScope :
	public Reflex::Object,
	public GLX::AnimationScope
{
	REFLEX_OBJECT(AnimationScope, Reflex::Object);

	static AnimationScope null;

	AnimationScope(UInt8 value)
		: GLX::AnimationScope(True(value), value == 2)
	{
	}

	AnimationScope(Reflex::NoValue)
		: GLX::AnimationScope(true)
	{
		m_previous = true;
	}
};

AnimationScope AnimationScope::null = { kNoValue };

struct CustomAnimationImpl : public GLX::Animation
{
	CustomAnimationImpl(VM::Context & context, VM::StackArgs <VM::Detail::FnObject&,VM::Detail::FnObject&,VM::Detail::FnObject&> & args)
		: context(context),
		m_onbegin(args.a),
		m_onprocess(args.b),
		m_onend(args.c)
	{
	}

	void OnBegin() override
	{
		if (!m_onbegin->DataReleased())
		{
			VM::CallReturningVoid(context, m_onbegin);
		}
	}

	bool OnClock(Float delta) override
	{
		if (m_onprocess->DataReleased())
		{
			return false;
		}
		else
		{
			return VM::CallReturningValue<bool>(context, m_onprocess, delta);
		}
	}

	void OnSkip() override
	{
		if (!m_onend->DataReleased())
		{
			VM::CallReturningVoid(context, m_onend);
		}
	}

	VM::Context & context;

	Reference <VM::Detail::FnObject> m_onbegin, m_onprocess, m_onend;
};

void BindAnimation(VM::Compiler::State & compilestate, Key32 ns, VM::TypeRef dynamic_t, VM::TypeRef object_t)
{
	REFLEX_USE(VM);

	auto bindings = compilestate.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto uint8_t = bindings->uint8_t;

	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;


	auto animation_scope_t = RegisterObject<AnimationScope>(bindings, kGLX, "AnimationScope");

	AddConstructor(bindings, animation_scope_t, { uint8_t }, [](Context & context)
	{
		VM_POP1(UInt8);

		VM_RTN(REFLEX_CREATE(AnimationScope, arg));
	});


	auto animation_t = RegisterObject<GLX::Animation>(bindings, kGLX, "Animation");

	VM::Detail::BindObjectMethod<&GLX::Animation::Play>(bindings, animation_t, "Play");

	auto voidfn_t = bindings->callback_t;

	auto processfn_t = compilestate.InstantiateTemplateType(kFn, { bool_t, float32_t });

	AddConstructor(bindings, animation_t, { voidfn_t, processfn_t, voidfn_t }, [](Context & context)
	{
		VM_POP(VM::Detail::FnObject&, VM::Detail::FnObject&, VM::Detail::FnObject&);

		VM_RTN(REFLEX_CREATE(CustomAnimationImpl, context, args));
	});


	auto animationcontainer_t = RegisterObject<GLX::ContainerAnimation>(bindings, kGLX, "ContainerAnimation");

	VM::Detail::BindObjectMethod<&GLX::ContainerAnimation::Clear>(bindings, animationcontainer_t, "Clear");


	VM::Detail::BindObjectMethod<&GLX::Animation::SetTarget>(bindings, animation_t, "SetTarget");

	VM::Detail::BindObjectMethod<&GLX::Animation::SetTime>(bindings, animation_t, "SetTime");


	auto interpolatedanimation_t = RegisterObject<GLX::InterpolatedAnimation>(bindings, kGLX, "InterpolatedAnimation");

	AddMethod(bindings, "SetEasing", void_t, { interpolatedanimation_t, key32_t }, [](Context & context)
	{
		VM_POP(GLX::InterpolatedAnimation&,UInt32);

		args.a.SetEasing(GLX::Detail::GetEasing(args.b, GLX::InterpolatedAnimation::kLinear));
	});
	
	VM::Detail::BindObjectMethod<&GLX::InterpolatedAnimation::Flip>(bindings, interpolatedanimation_t, "Flip");


	bindings->RegisterFunction(kGLX, "Add", void_t, { animationcontainer_t, animation_t }, {}, {}, VM::kMemberFunction | VM::ExternalFunction::kFlagsVaradic, [](Context & context)
	{
		auto args = PopVaradic<GLX::Animation*>(context, 1);

		auto & self = VM::Detail::Pop<GLX::ContainerAnimation&>(context.stack);

		for (auto & i : args)
		{
			self.Add(*i);
		}
	});
}

REFLEX_END

REFLEX_SET_TRAIT(GLXVM::AnimationScope, IsSingleThreadExclusive); 
REFLEX_SET_TRAIT(GLXVM::AnimationScope, IsNonCircular);
