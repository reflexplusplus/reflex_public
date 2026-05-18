#pragma once

#include "../compiler.h"




//
//

#define VM_TBIND_X(callback, env, ns, name, flags, ntarg, ...) env.RegisterTemplateFunction(ns, name, ntarg, {__VA_ARGS__}, callback, flags)

#define VM_TBIND(callback, env, ns, name, ntarg, ...) VM_TBIND_X(callback, env, ns, name, 0, ntarg, __VA_ARGS__)

#define VM_TBIND_VARADIC(callback, env, ns, name, ntarg, ...) VM_TBIND_X(callback, env, ns, name, VM::ExternalFunction::kFlagsVaradic, ntarg, __VA_ARGS__)

