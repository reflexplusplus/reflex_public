#pragma once

#include "detail/traits.h"
#include "detail/invoke.h"
#include "../detail/functions/allocate.h"
#if !defined(REFLEX_ARCH_WEBASM)
#include <functional>
#endif




//
//Primary API

namespace Reflex
{

	template <class SIG> class Function;	//callable function object, native implementation for web compatibility

}




//
//Function 

template <class RTN, class... ARGS>
class Reflex::Function<RTN(ARGS...)>
{
public:

	using Signature = RTN(ARGS...);

	template <class CALLABLE> static constexpr bool kBindable = [] { if constexpr (kIsType<std::decay_t<CALLABLE>, Function>) return false; else return std::is_copy_constructible_v<std::decay_t<CALLABLE>> && std::is_invocable_r_v<RTN, std::decay_t<CALLABLE> &, ARGS...>; }();	//AGENTS keep on one line, this is an implementation detail and should not distract from class overview


	//lifetime
	
	Function() requires (!kIsReference<RTN>);

	template <class CALLABLE> Function(CALLABLE && callable) requires (kBindable<CALLABLE>);

	Function(const Function & fn);

	Function(Function && fn) noexcept;

	Function(nullptr_t) = delete;

	~Function();



	//invoke

	RTN operator()(ARGS... args) const;

	template <class... VARGS> decltype(auto) Invoke(VARGS && ... args) const;



	//assign

	void Clear() noexcept requires (!kIsReference<RTN>);

	
	Function & operator=(const Function & fn);

	Function & operator=(Function && fn) noexcept;

	template <class CALLABLE> Function & operator=(CALLABLE && callable) requires (kBindable<CALLABLE>);

	Function & operator=(nullptr_t) = delete;


	explicit operator bool() const { return m_target != nullptr; }



private:
	
	struct Operations
	{
		RTN (*invoke)(void *, ARGS && ...);
		void (*destroy)(void *);
		void (*copy)(const void *, Function &);
		void (*move)(void *, void *);
	};

	template <class CALLABLE>
	struct Binder
	{
		static RTN Invoke(void * p, ARGS && ... args);
		static void Destroy(void * p);
		static void Copy(const void * p, Function & fn);
		static void Move(void * source, void * destination);
	};

	template <class CALLABLE> void Assign(CALLABLE && callable);

	void MoveFrom(Function & fn) noexcept;

	void Destroy() noexcept;

	
	const Operations * m_operations = nullptr;

	void * m_target = nullptr;

	alignas(16) unsigned char m_storage[32];


	template <class CALLABLE> static constexpr Operations st_operations = 
	{ 
		.invoke = &Binder<CALLABLE>::Invoke, 
		.destroy = &Binder<CALLABLE>::Destroy,
		.copy = &Binder<CALLABLE>::Copy,
		.move = &Binder<CALLABLE>::Move
	};

	static const Operations st_empty_operations;
};

REFLEX_SET_TRAIT_TEMPLATED(Function, IsBoolCastable);




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE> inline constexpr bool kIsStdFunction = false;

#if !defined(REFLEX_ARCH_WEBASM)
template <class RTN, class... ARGS> inline constexpr bool kIsStdFunction<std::function<RTN(ARGS...)>> = true;
#endif

inline void * AllocateFunctionStorage(UIntNative size, UIntNative alignment)
{
	// Default allocation is 16-byte aligned. Extra space preserves arbitrary capture alignment and stores the backing pointer for matching free.
	UIntNative padding = alignment - 1 + sizeof(void *);
	void * raw = g_default_allocator->Allocate(size + padding, AllocInfo());
	REFLEX_ASSERT(raw);
	UIntNative address = (ToUIntNative(raw) + sizeof(void *) + alignment - 1) & ~(alignment - 1);
	auto pointer = ToPointer<void>(address);
	reinterpret_cast<void**>(pointer)[-1] = raw;
	return pointer;
}

REFLEX_END

template <class RTN, class... ARGS> inline const typename Reflex::Function<RTN(ARGS...)>::Operations Reflex::Function<RTN(ARGS...)>::st_empty_operations =
{
	.invoke = []() -> FunctionPointer<RTN(void *, ARGS && ...)>
	{
		if constexpr (kIsReference<RTN>) return nullptr;
		else return [](void *, ARGS && ...) -> RTN { return Detail::NullReturn<RTN>::Get(); };
	}(),
	.destroy = [](void *) {},
	.copy = [](const void *, Function &) {},
	.move = [](void *, void *) {}
};

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function() requires (!kIsReference<RTN>)
{
	m_operations = &st_empty_operations;
}

template <class RTN, class... ARGS> template <class CALLABLE> inline Reflex::Function<RTN(ARGS...)>::Function(CALLABLE && callable) requires (kBindable<CALLABLE>)
{
	Assign(std::forward<CALLABLE>(callable));
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function(const Function & fn)
{
	if (fn.m_target) fn.m_operations->copy(fn.m_target, *this);
	else m_operations = fn.m_operations;
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function(Function && fn) noexcept
{
	MoveFrom(fn);
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::~Function()
{
	Destroy();
}

template <class RTN, class... ARGS> inline void Reflex::Function<RTN(ARGS...)>::Clear() noexcept requires (!kIsReference<RTN>)
{
	Destroy();
	m_operations = &st_empty_operations;
}

template <class RTN, class... ARGS> inline RTN Reflex::Function<RTN(ARGS...)>::operator()(ARGS... args) const
{
	REFLEX_ASSERT(m_operations && m_operations->invoke);

	return m_operations->invoke(m_target, std::forward<ARGS>(args)...);
}

template <class RTN, class... ARGS> template <class... VARGS> inline decltype(auto) Reflex::Function<RTN(ARGS...)>::Invoke(VARGS && ... args) const
{
	return operator()(std::forward<VARGS>(args)...);
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(const Function & fn)
{
	if (this != &fn) 
	{ 
		Function copy(fn); 
		*this = std::move(copy); 
	}

	return *this;
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(Function && fn) noexcept
{
	if (this != &fn) 
	{ 
		Destroy(); 
		MoveFrom(fn); 
	}

	return *this;
}

template <class RTN, class... ARGS> template <class CALLABLE> inline Reflex::Function<RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(CALLABLE && callable) requires (kBindable<CALLABLE>)
{
	Function replacement(std::forward<CALLABLE>(callable));
	
	return *this = std::move(replacement);
}

template <class RTN, class... ARGS> template <class CALLABLE> inline void Reflex::Function<RTN(ARGS...)>::Assign(CALLABLE && callable)
{
	using Input = std::remove_reference_t<CALLABLE>;
	using Type = std::decay_t<CALLABLE>;

	if constexpr (std::is_pointer_v<Input> || std::is_member_pointer_v<Input> || Detail::kIsStdFunction<std::remove_cv_t<Input>>)
	{
		if (!callable)
		{
			REFLEX_ASSERT(false);
			return;
		}
	}

	constexpr bool kSmall = sizeof(Type) <= sizeof(m_storage) && alignof(Type) <= 16 && std::is_nothrow_move_constructible_v<Type>;

	void * target;
	
	if constexpr (kSmall)
	{
		target = m_storage;
	}
	else
	{
		target = Detail::AllocateFunctionStorage(sizeof(Type), alignof(Type));
	}

	::new (target) Type(std::forward<CALLABLE>(callable));
	
	m_target = target;
	m_operations = &st_operations<Type>;
}

template <class RTN, class... ARGS> inline void Reflex::Function<RTN(ARGS...)>::MoveFrom(Function & fn) noexcept
{
	m_operations = fn.m_operations;

	if (fn.m_target == fn.m_storage)
	{
		m_target = m_storage;
		m_operations->move(fn.m_target, m_target);
	}
	else
	{
		m_target = fn.m_target;
	}

	fn.m_target = nullptr;
	fn.m_operations = nullptr;
}

template <class RTN, class... ARGS> template <class CALLABLE> inline RTN Reflex::Function<RTN(ARGS...)>::Binder<CALLABLE>::Invoke(void * p, ARGS && ... args)
{
	if constexpr (kIsType<RTN,void>)
	{
		Detail::InvokeCallable(*Cast<CALLABLE>(p), std::forward<ARGS>(args)...);
	}
	else
	{
		return Detail::InvokeCallable(*Cast<CALLABLE>(p), std::forward<ARGS>(args)...);
	}
}

template <class RTN, class... ARGS> template <class CALLABLE> inline void Reflex::Function<RTN(ARGS...)>::Binder<CALLABLE>::Destroy(void * p)
{
	Detail::Constructor<CALLABLE>::Destruct(Cast<CALLABLE>(p));
}

template <class RTN, class... ARGS> template <class CALLABLE> inline void Reflex::Function<RTN(ARGS...)>::Binder<CALLABLE>::Copy(const void * p, Function & fn)
{
	fn.Assign(*Cast<CALLABLE>(p));
}

template <class RTN, class... ARGS> template <class CALLABLE> inline void Reflex::Function<RTN(ARGS...)>::Binder<CALLABLE>::Move(void * source, void * destination)
{
	if constexpr (std::is_move_constructible_v<CALLABLE>)
	{
		::new (destination) CALLABLE(std::move(*Cast<CALLABLE>(source)));
	}
	else
	{
		::new (destination) CALLABLE(*Cast<CALLABLE>(source));
	}

	Binder<CALLABLE>::Destroy(source);
}

template <class RTN, class... ARGS> inline void Reflex::Function<RTN(ARGS...)>::Destroy() noexcept
{
	if (m_target)
	{
		m_operations->destroy(m_target);

		if (m_target != m_storage)
		{
			g_default_allocator->Free(reinterpret_cast<void **>(m_target)[-1]);
		}
	}

	m_operations = nullptr;
	m_target = nullptr;
}
