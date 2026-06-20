#include "../../../../include/reflex/glx/animation.h"




//
//global

void Reflex::GLX::Run(Object & object, Key32 id, TRef <Animation> animation)
{
	object.SetProperty(id, animation);

	animation->SetTarget(object);

	animation->Play();
}

void Reflex::GLX::Run(Object & object, Key32 id, Float32 time, TRef <Animation> animation)
{
	object.SetProperty(id, animation);

	animation->SetTarget(object);

	animation->SetTime(time);

	animation->Play();
}

void Reflex::GLX::Run(Object & object, Key32 id, Float32 time, InterpolatedAnimation::Easing easing, TRef <InterpolatedAnimation> animation)
{
	object.SetProperty(id, animation);

	animation->SetTarget(object);

	animation->SetTime(time);

	animation->SetEasing(easing);

	animation->Play();
}

void Reflex::GLX::Stop(Object & object, Key32 id)
{
	object.UnsetProperty<Animation>(id);
}
