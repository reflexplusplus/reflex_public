#pragma once

#include "compilerimpl.h"




//
//

//TODO return Variable which includes const info

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct CompilerImpl::ExpressionHandler
{
	inline static UInt whatever = 0;

	struct RenderScope : public Scope
	{
		RenderScope(Scope & parent, Instructions & output)
			: CompilerImpl::Scope(parent.state, parent.IsGlobal(), true, &parent, parent.stackframelayout, m_variables, output, parent.GetNamespace()),
			m_variables(parent.allocator)
		{
		}

		Variable Parse2(Source & src, Itr && itr, TypeRef lhs)
		{
			return ExpressionHandler::ParseX(src, itr, *this, ExpressionHandler::kContextTemporary, lhs);
		}

		Array < Pair <Symbol,Variable> > m_variables;

	};

	enum Context
	{
		kContextTemporary,
		kContextStatement,		//normal 'line' eg "Int32 a = b";
		kContextConditional,	//temporary, but allows variable declaration. example "if (auto b = 1)"
		kNumContext
	};



	//parse

	//TODO should use this, then can prevent const being passed by ref.  needs rework of autocast to use Variable

	static Variable ParseX(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs);

	static TypeRef Parse(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs);



	//casting

	static bool CanCast(Source & src, Scope & scope, const Variable & variable, TypeRef target, UInt & castcount = whatever);

	static Variable ExpectCast(Source & src, Scope & scope, const Variable & variable, TypeRef target, UInt & castcount = whatever);



	//secondary

	static Variable ResolveTemporary(Source & src, Scope & scope, const Variable & variable, bool byref);

	static void ResolveIfObjectTemporary(Source & src, Scope & scope, Variable & variable);


	struct Private;

};

REFLEX_END_INTERNAL
