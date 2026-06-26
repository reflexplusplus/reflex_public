#include "[require].h"




REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

template <bool RECURSIVE>
struct CriticalSectionImpl : public CriticalSection
{
	virtual void Enter() override;

	virtual void Leave() override;

	ConditionalType <RECURSIVE,std::recursive_mutex,std::mutex> m_mutex;
};

template <bool RECURSIVE> void CriticalSectionImpl<RECURSIVE>::Enter()
{
	m_mutex.lock();
}

template <bool RECURSIVE> void CriticalSectionImpl<RECURSIVE>::Leave()
{
	m_mutex.unlock();
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::CriticalSection> Reflex::System::CriticalSection::Create(bool recursive, Allocator & allocator)
{
	if (recursive)
	{
		return REFLEX_CREATE_EX(allocator, Common::CriticalSectionImpl<true>);
	}
	else
	{
		return REFLEX_CREATE_EX(allocator, Common::CriticalSectionImpl<false>);
	}
}
