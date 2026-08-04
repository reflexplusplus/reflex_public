#pragma once

#include "[require].h"




//
//

#define REFLEX




//
//assert

#define REFLEX_STATIC_ASSERT(...) static_assert(bool(__VA_ARGS__), REFLEX_STRINGIFY(__VA_ARGS__));




//
//text

#define REFLEX_CONCATENATE(a, b) REFLEX_CONCATENATE_IMPL(a,b)

#define REFLEX_STRINGIFY(...) REFLEX_STRINGIFY_IMPL(__VA_ARGS__)




//
//meta

#define REFLEX_NONCOPYABLE(T) T(const T &) = delete; T & operator=(const T &) = delete;




//
//macros

#if REFLEX_DEBUG
#define REFLEX_INLINE inline
#else
#define REFLEX_INLINE REFLEX_FORCEINLINE
#endif


#define REFLEX_OFFSETOF(TYPE,MEMBER) offsetof(TYPE,MEMBER)


#define REFLEX_MARKER(X) X:


#define REFLEX_LOOP_TYPE(TYPE, VAR, N) for (TYPE VAR = TYPE(0), _##VAR##n = N; VAR < _##VAR##n; ++VAR)

#define REFLEX_LOOP(VAR, N) REFLEX_LOOP_TYPE(Reflex::UInt32, VAR, N)

#define REFLEX_RLOOP(VAR, N) for (Reflex::UInt32 VAR = (N - 1); VAR < ~Reflex::UInt32(0); --VAR)


#define REFLEX_LOOP_PTR(ADR, VAR, N) for (auto VAR = ADR, end_ = ADR + (N); VAR < end_; ++VAR)

#define REFLEX_RLOOP_PTR(ADR, PTR, N) for (auto start_ = ADR, PTR = start_ + (N); PTR-- > start_;)


#define REFLEX_FOREACH(I, CONTAINER) for ([[maybe_unused]] auto & I : CONTAINER)

#define REFLEX_RFOREACH(I, CONTAINER) for ([[maybe_unused]] auto & I : Reverse(CONTAINER))




//
//namespace

#define REFLEX_USE(x) using namespace x;

#define REFLEX_USE_ENUM(NS, ENUM) using ENUM = NS::ENUM; using enum NS::ENUM;


#define REFLEX_NS(NS) namespace NS {

#define REFLEX_END };


#define REFLEX_BEGIN_INTERNAL(NS) namespace NS { namespace {

#define REFLEX_END_INTERNAL }}


#define REFLEX_LOCAL(RTN, NAME) struct NAME { static RTN Call

#define REFLEX_INLINE_LOCAL(RTN, NAME) struct NAME { static REFLEX_INLINE RTN Call




//
//dev

#if (REFLEX_DEBUG)

#define REFLEX_IF_DEBUG(...) __VA_ARGS__
#define REFLEX_SHOWSTOPPER(...)

#else

#define REFLEX_IF_DEBUG(...)
#define REFLEX_SHOWSTOPPER(...) REFLEX_STATIC_ASSERT(false)

#endif




//
//impl

#define REFLEX_CONCATENATE_IMPL(a, b) a##b

#define REFLEX_STRINGIFY_IMPL(...) #__VA_ARGS__