#pragma once

#include "[require].h"




//
//Experimental API

namespace Reflex::VM
{

	class Module;

	class Compiler;

	class Bindings;

	class Program;

	class Context;

	struct Source;


	struct Type;

	using TypeRef = const Type *;


	struct Argument;

	struct Function;

	struct Intrinsic;

	struct ExternalFunction;

	struct ScriptFunction;


	struct ExternalObject;


	using Symbol = Pair <Key32>;

	using StaticString = CString::View;


	enum ContextFlags : UInt8
	{
		kContextFlagMain = 1,
		kContextFlagRt = 2,
		kContextFlagUi = 4,
		kContextFlagCustom = 8,

		kContextFlagMainBg = 16,
		kContextFlagRtBg = 32,
		kContextFlagUiBg = 64,
		kContextFlagCustomBg = 128
	};

	constexpr Key32 kGlobal = kHashSeed;

}