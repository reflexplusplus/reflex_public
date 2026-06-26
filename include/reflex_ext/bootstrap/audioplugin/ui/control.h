#pragma once

#include "../audioplugin.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class ParamControl;	//widget to edit a parameter

}




//
//ParamControl

class Reflex::Bootstrap::ParamControl : public GLX::Object
{
public:

	[[nodiscard]] static TRef <ParamControl> Create(AudioPlugin & instance, UInt param_idx);

	[[nodiscard]] static TRef <ParamControl> Create(ConstTRef <ParamDesc> param_desc, const Value32 & value);
};

REFLEX_SET_TRAIT(Bootstrap::ParamControl, IsAbstract);
