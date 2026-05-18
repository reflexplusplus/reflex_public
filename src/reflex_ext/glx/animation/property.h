#pragma once

#include "animation.h"




//
//

REFLEX_NS(Reflex::GLX)

class AbstractPropertyAnimation;

REFLEX_END




//
//AbstractPropertyAnimation

class Reflex::GLX::AbstractPropertyAnimation : public InterpolatedAnimationImpl
{
public:

	struct TypeHandler
	{
		const TypeID & type_id;
		FunctionPointer <TRef<Reflex::Object>()> create;
		UInt8 offset;
		UInt8 n;
	};

	static const TypeHandler kTypeHandlers[5];

	REFLEX_INLINE static Float * GetValueAdr(const AbstractPropertyAnimation::TypeHandler & type, Reflex::Object & object)
	{
		return Reinterpret<Float>(Reinterpret<UInt8>(&object) + type.offset);
	}



protected:

	AbstractPropertyAnimation(const TypeHandler & type, Key32 property_id);


	virtual void OnSetTarget(GLX::Object & object) override;

	virtual void OnFlip() override;

	virtual Float32 OnRecomputeProgress(GLX::Object & target) override;

	template <UInt N> static void OnInterpolate(AbstractPropertyAnimation & self, GLX::Object & object, Float x);

	static InterpolateFn GetInterpolateFn(UInt n);


	const TypeHandler type;

	const Key32 property_id;

	Reference <Reflex::Object> m_prop;

	Float32 * m_ptr;

	Float32 m_from_to[1];
};
