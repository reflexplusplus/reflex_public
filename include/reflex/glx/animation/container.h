#pragma once

#include "animation.h"




//
//Primary API

namespace Reflex::GLX
{

	class ContainerAnimation;

}




//
//ContainerAnimation

class Reflex::GLX::ContainerAnimation : public Animation
{
public:

	REFLEX_OBJECT(ContainerAnimation, Animation);

	static ContainerAnimation & null;

	

	//types
	
	using Reference = Reflex::Reference <Animation,kReferenceStrictSafeFlags>;



	//access

	void Clear();

	void Add(Animation & scene);



protected:

	//callbacks

	ContainerAnimation();

	virtual void OnClear() {}


	ArrayView <Reference> GetScenes() { return m_scenes; }


	void Reset(Animation & scene) { scene.Reset(); }

	bool Process(Animation & scene, Float32 delta) { return scene.Process(delta); }


	void OnSetTarget(GLX::Object & object) override;



private:

	Array <Reference> m_scenes;

	Animation * m_first;

};

REFLEX_SET_TRAIT(Reflex::GLX::ContainerAnimation, IsSingleThreadExclusive)
