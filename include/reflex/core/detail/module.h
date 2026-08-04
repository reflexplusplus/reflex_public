#pragma once

#include "initialiser.h"




#define REFLEX_MODULE_NULL_INSTANCE(NS, MODULE, IMPL, VAR) namespace NS { Reflex::Detail::Module::Member <IMPL> VAR(MODULE); }
#define REFLEX_MODULE_NULL(NS, MODULE, TYPE, IMPL) REFLEX_MODULE_NULL_INSTANCE(NS,MODULE,IMPL,g##_null_##TYPE); REFLEX_INSTANTIATE_EXTERN_NULL(NS::TYPE, NS::g##_null_##TYPE)




//
//Detail

namespace Reflex::Detail
{

	class Module;

}




//
//Detail::Module

class Reflex::Detail::Module
{
public:

	REFLEX_NONCOPYABLE(Module);


	class AbstractMember;

	template <class TYPE> class Member;


	Module(const char * name);

	Module(const char * name, const Module & parent);

	virtual ~Module() = default;
	

	void Init();

	void Deinit();


	bool IsInitalised() const { return m_is_initalised; }



private:

	bool m_is_initalised;	//static variables are zero-initialised

	mutable UInt8 m_num_module, m_num_member;
	
	mutable Int8 m_ordered_members;

	mutable Module * m_first_module;

	AbstractMember * m_first_member;

	Module * m_next;

	REFLEX_IF_DEBUG(const char * m_name;)
};




//
//Detail::Module::AbstractMember

class Reflex::Detail::Module::AbstractMember
{
public:

	REFLEX_NONCOPYABLE(AbstractMember);


	AbstractMember(Module & module, Int8 priority = 0);	//AbstractModule must already be instantiantiaed, and in same TU so that it is guaranteed to be ordered after

	virtual ~AbstractMember() = default;



private:

	friend Module;

	virtual void OnInit() = 0;

	virtual void OnDeinit() = 0;

	
	AbstractMember * m_next;

	const Int8 m_priority;
};




//
//Detail::Module::Member

template <class TYPE>
class Reflex::Detail::Module::Member :
	public AbstractMember,
	public Detail::Initialiser <TYPE>
{
public:

	using AbstractMember::AbstractMember;



private:

	void OnInit() final;

	void OnDeinit() final;
};




//
//impl

inline Reflex::Detail::Module::Module(const char * name)
	REFLEX_IF_DEBUG(: m_name(name))
{
}

inline Reflex::Detail::Module::Module(const char * name, const Module & parent)
	: m_next(parent.m_first_module)
	REFLEX_IF_DEBUG(, m_name(name))
{
	parent.m_first_module = this;
	parent.m_num_module++;
}

inline Reflex::Detail::Module::AbstractMember::AbstractMember(Module & module, Int8 priority)
	: m_next(module.m_first_member)
	, m_priority(priority)
{
	module.m_first_member = this;
	module.m_num_member++;
	module.m_ordered_members |= priority;
}

template <class TYPE> inline void Reflex::Detail::Module::Member<TYPE>::OnInit()
{
	Detail::Initialiser<TYPE>::Init();
}

template <class TYPE> inline void Reflex::Detail::Module::Member<TYPE>::OnDeinit()
{
	Detail::Initialiser<TYPE>::Deinit();
}
