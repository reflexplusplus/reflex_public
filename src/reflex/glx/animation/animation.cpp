#include "player.h"
#include "../library.h"




//
//animation

const decltype(&Reflex::GLX::Animation::Begin) Reflex::GLX::Animation::kOnProcess[2] = { &Animation::Skip<true>, &Animation::OnClock };

Reflex::GLX::Animation::Animation()
	: m_callback(&Animation::Begin)
{
}

bool Reflex::GLX::Animation::Begin(Float delta)
{
	REFLEX_ASSERT(GetRetainCount());	//ensure held by a lit

	REFLEX_ASSERT(!delta);

	auto retain = AutoRelease(this);

	m_callback = kOnProcess[AnimationScope::IsEnabled()];

	OnBegin();

	return (this->*m_callback)(delta);
}

template <bool CALL> bool Reflex::GLX::Animation::Skip(Float delta)
{
	if constexpr (CALL)
	{
		REFLEX_ASSERT(GetRetainCount());	//ensure held by a lit

		auto retain = AutoRelease(this);

		OnSkip();

		m_callback = &Animation::Skip<false>;
	}

	return false;
}

void Reflex::GLX::Animation::Reset()
{
	m_callback = &Animation::Begin;

	Process(0.0f);
}

void Reflex::GLX::Animation::Play()
{
	REFLEX_ASSERT(Or(GetAllocator(), IsNull(*this)));

	auto & player = g_library->m_globalplayer.Run();

	player.m_animations.Set(this, this);

	Reset();
}

const Reflex::UInt32 & Reflex::GLX::Detail::GetNumActiveAnimation()
{
	return GlobalPlayer::st_nactive;
}