#include "array.h"
#include "arrayiterator.h"
#include "stream.h"




REFLEX_BEGIN_INTERNAL(Reflex::VM)

typedef UInt32 Index32;

struct ValueAllocation : public Allocation
{
	REFLEX_OBJECT(ValueAllocation, Allocation);

	ValueAllocation(UInt stride, UInt size)
		: Allocation(size)
	{
		RemoveConst(reserved) = ToPointer<void>(stride);
	}
};

struct ObjectAllocation : public Allocation
{
	REFLEX_OBJECT(ObjectAllocation, Allocation);

	ObjectAllocation(Context & context, UInt size)
		: Allocation(size)
	{
	}

	~ObjectAllocation()
	{
		REFLEX_LOOP_PTR(Reinterpret<Object*>(data + 0), i, size)
		{
			(**i).ReleaseMt();
		}
	}
};

struct CircularObjectAllocation : public ObjectAllocation
{
	REFLEX_OBJECT(CircularObjectAllocation, Allocation);

	CircularObjectAllocation(Context & context, UInt size)
		: ObjectAllocation(context, size)
	{
		RemoveConst(Allocation::reserved) = REFLEX_CREATE(ObjectOf<Detail::Circular>, context, this);

		Retain(Reinterpret<Object>(Allocation::reserved));
	}

	~CircularObjectAllocation()
	{
		Release(Reinterpret<Object>(Allocation::reserved));
	}

	virtual void OnReleaseData() override
	{
		REFLEX_LOOP_PTR(Reinterpret<Object*>(data + 0), i, size)
		{
			Reflex::Detail::SetReferenceCountedPointer(*i, &REFLEX_NULL(Object));
		}

		REFLEX_NULL(Object).GetActualRetainCount() += size;
	}
};

struct CircularObjectArray : 
	public ObjectArray,
	public Detail::Circular
{
	CircularObjectArray(Context & context, TypeRef array_t, TypeRef value_t)
		: ObjectArray(context, array_t, value_t),
		Detail::Circular(context, this)
	{
	}

	TRef <Allocation> CreateAllocation(Context & context, UInt size) override
	{
		return Reflex::Detail::Constructor<CircularObjectAllocation>::CreateVariableSize(g_default_allocator, size * sizeof(Detail::Pointer), context, size);
	}

	virtual bool SetData(Allocation & allocation) override
	{
		if (DynamicCast<CircularObjectAllocation>(allocation))
		{
			AbstractArray::SetDataExternal(allocation, allocation.data, allocation.size);

			return true;
		}

		return false;
	}

	virtual void OnReleaseData() override
	{
		m_allocation.Clear();

		m_size = 0;

		m_wrap = 1;

		m_capacity = 0;

		m_ptr = null.Adr();
	}
};

inline constexpr UInt32 kMaxSizeMask = 0xFFFFFF;

typedef Detail::Value32 Value32;

template <class ARRAY>
struct IsObjectArray
{
	static const bool value = IsType<ARRAY,ObjectArray,CircularObjectArray>::value;
};

enum ArrayOptimisation
{
	kArrayOptimisationNone,
	kArrayOptimisationZeroInit,
	kArrayOptimisationValue32,
	kArrayOptimisationObjectSt,
};

Object * ArrayContextCopyMt(Context & context, Object & object, TypeRef array_t, bool mt)
{
	auto copy = Cast<AbstractArray>(VM::Detail::CreateObject(context, array_t));

	auto self = Cast<AbstractArray>(object);

	copy->SetDataExternal(self->m_allocation, self->m_ptr, self->m_size);

	return copy.Adr();
}

template <class ARRAY> REFLEX_INLINE void * GetNull(ARRAY & array)
{
	if constexpr (IsObjectArray<ARRAY>::value)
	{
		return array.null.Adr();
	}
	else
	{
		return array.null;
	}
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE UInt GetStride(const ValueArray & array)
{
	if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		return sizeof(Value32);
	}
	else
	{
		return array.m_stride;
	}
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE constexpr UInt GetStride(const ObjectArray & array)
{
	return sizeof(Detail::Pointer);
}

template <ArrayOptimisation OPTIMISATION, class ARRAY> Data::Archive::View GetRawView(const ARRAY & array)
{
	return { Reinterpret<UInt8>(array.m_ptr), UInt(array.m_size * GetStride<OPTIMISATION>(array)) };
}

template <ArrayOptimisation OPTIMISATION, class ARRAY> UInt GetByteSize(const ARRAY & array)
{
	return array.m_size * GetStride<OPTIMISATION>(array);
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE void SetRegion(ValueArray & self, UInt idx, UInt n, void * pvalue)
{
	if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		auto ptr = Reinterpret<Value32>(self.m_ptr) + idx;

		auto end = ptr + n;

		Value32 value32 = *Reinterpret<Value32>(pvalue);

		while (ptr < end)
		{
			*ptr++ = value32;
		}
	}
	else
	{
		auto stride = GetStride<OPTIMISATION>(self);

		auto ptr = Reinterpret<UInt8>(self.m_ptr) + (idx * stride);

		auto end = ptr + (n * stride);

		while (ptr < end)
		{
			MemCopy(pvalue, ptr, stride);

			ptr += stride;
		}
	}
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE void SetRegion(ObjectArray & self, UInt start, UInt n, void * null)
{
	REFLEX_LOOP_PTR(Reinterpret<Object*>(self.m_ptr) + start, i, n)
	{
		Reflex::Detail::SetReferenceCountedPointer(*i, Cast<Object>(null));
	}
}

template <ArrayOptimisation OPTIMISATION, class ARRAY> REFLEX_INLINE void WipeRegion(ARRAY & self, UInt start, UInt n)
{
	if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		MemClear(Reinterpret<Value32>(self.m_ptr) + start, n * sizeof(Value32));
	}
	else if constexpr (OPTIMISATION == kArrayOptimisationZeroInit)
	{
		auto stride = GetStride<OPTIMISATION>(self);

		MemClear(Reinterpret<UInt8>(self.m_ptr) + (start * stride), n * stride);
	}
	else
	{
		SetRegion<OPTIMISATION>(self, start, n, GetNull(self));
	}
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE void ConstructRegion(ValueArray & self, UInt start, UInt n)
{
	WipeRegion<OPTIMISATION>(self, start, n);
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE void ConstructRegion(ObjectArray & self, UInt start, UInt n)
{
	auto null = Cast<Object>(self.null.Adr());

	REFLEX_LOOP_PTR(Reinterpret<Object*>(self.m_ptr) + start, i, n)
	{
		*i = null;
	}
	
	null->GetActualRetainCount() += n;
}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE void DestructRegion(ValueArray & self, UInt start, UInt n) {}

template <ArrayOptimisation OPTIMISATION> REFLEX_INLINE void DestructRegion(ObjectArray & self, UInt start, UInt n)
{
	auto null = Cast<Object>(self.null.Adr());

	SetRegion<OPTIMISATION>(self, start, n, null);
}

template <class ARRAY> inline ARRAY * CreateEmptyArrayImpl(Context & context, TypeRef array_t, TypeRef value_t)
{
	if constexpr (IsObjectArray<ARRAY>::value)
	{
		return REFLEX_CREATE(ARRAY, context, array_t, value_t);
	}
	else
	{
		return Reflex::Detail::Constructor<ValueArray>::CreateVariableSize(g_default_allocator, value_t->size, array_t, value_t);
	}
}

REFLEX_INLINE UInt8 * RawAllocateImpl(Context & context, AbstractArray & array, Index32 capacity)
{
	auto allocation = array.CreateAllocation(context, capacity);

	auto data = Reinterpret<UInt8>(allocation->data);

	array.m_allocation = allocation;

	array.m_ptr = data;

	array.m_capacity = capacity;

	return data;
}

inline void UpdateSizeImpl(AbstractArray & array, Index32 size)
{
	array.m_size = size;

	array.m_wrap = size ? size : 1;
}

template <ArrayOptimisation OPTIMISATION, class ARRAY> inline void ReallocateImpl(Context & context, ARRAY & self, Index32 size)
{
	Reference <Object> current = self.m_allocation;
	
	auto previous_size = self.m_size;

	auto previous_data = self.m_ptr;

	auto capacity = Reflex::Detail::CalculateExpandedCapacity(size);

	UInt8 * data = RawAllocateImpl(context, self, capacity);
	
	MemCopy(previous_data, data, previous_size * GetStride<OPTIMISATION>(self));
	
	if constexpr (IsObjectArray<ARRAY>::value)
	{
		REFLEX_LOOP_PTR(Reinterpret<Object*>(previous_data), i, previous_size)
		{
			(**i).RetainMt();
		}

		ConstructRegion<OPTIMISATION>(self, previous_size, capacity - previous_size);
	}
}

template <ArrayOptimisation OPTIMISATION> void ValueArraySearch(Context & context)
{
	if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		VM_POP(ValueArray&,Value32,UInt32&);

		auto size = args.a.m_size;

		if (args.c < size)
		{
			ArrayView <Value32> a = { Reinterpret<Value32>(args.a.m_ptr) + args.c, size - args.c };

			if (auto idx = Search(a, args.b))
			{
				args.c += idx.value;

				VM_RTN(true);

				return;
			}
		}

		VM_RTN(false);
	}
	else
	{
		auto size = ReadFunctionData<UInt8>(context);

		auto args = Detail::Pop(context.stack, sizeof(Detail::Pointer) + size + sizeof(Detail::Pointer));

		auto & haystack = **Reinterpret<ValueArray*>(args);

		auto pvalue = args + sizeof(Detail::Pointer);

		auto & start = **Reinterpret<UInt32*>(pvalue + size);

		if (start < haystack.m_size)
		{
			auto ptest = Reinterpret<UInt8>(haystack.m_ptr) + (start * size);

			auto end = Reinterpret<UInt8>(haystack.m_ptr) + (haystack.m_size * size);

			while (ptest < end)
			{
				if (MemCompare(ptest, pvalue, size))
				{
					start = UInt(ptest - Reinterpret<UInt8>(haystack.m_ptr)) / size;

					return VM_RTN(true);
				}

				ptest += size;
			}
		}

		VM_RTN(false);
	}
}

template <ArrayOptimisation OPTIMISATION> void ValueArraySearchRegion(Context & context)
{
	VM_POP(ValueArray&,ValueArray&,UInt32&);

	if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		ArrayView <Value32> a = args.a.GetView<Value32>();

		ArrayView <Value32> b = args.b.GetView<Value32>();

		if (auto idx = Search(Mid<true>(a, args.c), b))
		{
			args.c += idx.value;

			VM_RTN(true);
		}
		else
		{
			VM_RTN(false);
		}
	}
	else
	{
		auto & haystack = args.a;
		auto & needle = args.b;
		auto start = args.c;

		if (start + needle.m_size <= haystack.m_size)
		{
			auto stride = GetStride<OPTIMISATION>(needle);

			Data::Archive::View needleview = GetRawView<OPTIMISATION>(needle);

			Data::Archive::View test = { Reinterpret<UInt8>(haystack.m_ptr) + (start * stride), needleview.size };

			auto end = Reinterpret<UInt8>(haystack.m_ptr) + ((haystack.m_size - needle.m_size) + 1) * stride;

			while (test.data < end)
			{
				if (test == needleview)
				{
					args.c = UInt(test.data - Reinterpret<UInt8>(haystack.m_ptr)) / stride;

					return VM_RTN(true);
				}

				test.data += stride;
			}
		}

		VM_RTN(false);
	}
}

void ValueArrayEqualImpl(Context & context)
{
	VM_POP(ValueArray&,ValueArray&);

	VM_RTN(GetRawView<kArrayOptimisationNone>(args.a) == GetRawView<kArrayOptimisationNone>(args.b));
}

void ValueArrayInequalImpl(Context & context)
{
	VM_POP(ValueArray&,ValueArray&);

	VM_RTN(GetRawView<kArrayOptimisationNone>(args.a) != GetRawView<kArrayOptimisationNone>(args.b));
}




//
//actual array functions

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void CreateWithArgsImpl(Context & context)
{
	auto n = GetNumberArgs(context);

	auto & [value_t,array_t] = ReadFunctionData<Pair<TypeRef>>(context);

	auto bytes = n * value_t->size;

	auto args = VM::Detail::Pop(context.stack, bytes);

	if (n)
	{
		auto self = CreateEmptyArrayImpl<ARRAY>(context, array_t, value_t);

		RawAllocateImpl(context, *self, n);

		UpdateSizeImpl(*self, n);

		if constexpr (IsObjectArray<ARRAY>::value)
		{
			auto dest = Cast<Object*>(self->m_ptr);

			REFLEX_LOOP_PTR(Reinterpret<Object*>(args), i, n)
			{
				Detail::Pointer pobject = *i;

				*dest++ = pobject;

				Retain(*pobject);
			}
		}
		else
		{
			MemCopy(args, self->m_ptr, bytes);
		}

		VM_RTN(self);
	}
	else
	{
		VM_RTN(array_t->ctr(context, array_t));
	}
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void CreateWithSizeImpl(Context & context)
{
	auto & [value_t,array_t] = ReadFunctionData<Pair<TypeRef>>(context);

	VM_POP1(UInt32);

	auto size = arg & kMaxSizeMask;

	auto self = CreateEmptyArrayImpl<ARRAY>(context, array_t, value_t);

	RawAllocateImpl(context, *self, size);

	UpdateSizeImpl(*self, size);

	ConstructRegion<OPTIMISATION>(*self, 0, size);

	VM_RTN(self);
}

template <class ARRAY> void CopyImpl(Context & context)
{
	auto & [value_t,array_t] = ReadFunctionData<Pair<TypeRef>>(context);

	VM_POP1(ARRAY&);

	auto rtn = CreateEmptyArrayImpl<ARRAY>(context, array_t, value_t);

	if (auto size = arg.m_size)
	{
		auto dest = RawAllocateImpl(context, *rtn, size);

		if constexpr (IsObjectArray<ARRAY>::value)
		{
			auto src = Reinterpret<Object*>(arg.m_ptr);

			REFLEX_LOOP_PTR(Reinterpret<Object*>(dest), ptr, size)
			{
				Detail::Pointer pobject = *src++;

				Retain(pobject);

				*ptr = pobject;
			}
		}
		else
		{
			MemCopy(arg.m_ptr, dest, size * arg.m_stride);
		}

		UpdateSizeImpl(*rtn, size);
	}

	VM_RTN(rtn);
}

void ShareImpl(Context & context)
{
	auto & [value_t,array_t] = ReadFunctionData<Pair<TypeRef>>(context);

	VM_POP1(AbstractArray&);

	auto rtn = Cast<AbstractArray>(&array_t->ctr(context, array_t));

	REFLEX_ASSERT(rtn->m_stride == arg.m_stride);

	rtn->m_allocation = arg.m_allocation;

	rtn->m_ptr = arg.m_ptr;

	rtn->m_capacity = arg.m_capacity;

	rtn->m_size = arg.m_size;

	rtn->m_wrap = arg.m_wrap;

	VM_RTN(rtn);
}

void ClearImpl(Context & context)
{
	VM_POP1(ValueArray&);

	arg.m_size = 0;

	arg.m_wrap = 1;
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void WipeImpl(Context & context)
{
	VM_POP1(ARRAY&);

	WipeRegion<OPTIMISATION>(arg, 0, arg.m_size);
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void FillImpl(Context & context)
{
	if constexpr (IsObjectArray<ARRAY>::value)
	{
		VM_POP(ARRAY&,Object*);

		auto & self = args.a;

		SetRegion<OPTIMISATION>(self, 0, self.m_size, args.b);
	}
	else if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		VM_POP(ARRAY&,Value32);

		auto & self = args.a;

		SetRegion<OPTIMISATION>(self, 0, self.m_size, &args.b);
	}
	else
	{
		auto size = ReadFunctionData<UInt8>(context);

		auto args = Detail::Pop(context.stack, sizeof(Detail::Pointer) + size);

		auto & self = **Reinterpret<ValueArray*>(args);

		SetRegion<kArrayOptimisationNone>(self, 0, self.m_size, args + sizeof(Detail::Pointer));
	}
}

void NudgeImpl(Context & context)
{
	VM_POP(AbstractArray&,UInt32);

	auto & self = args.a;

	auto n = Min(args.b, self.m_size);

	reinterpret_cast<UInt8*&>(self.m_ptr) += n * self.m_stride;

	self.m_capacity -= n;

	UpdateSizeImpl(self, self.m_size - n);
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void RemoveImpl(Context & context)
{
	VM_POP(ARRAY&,UInt32,UInt32);

	auto & self = args.a;

	auto idx = args.b;

	auto n = args.c;
	
	if (idx + n > self.m_size)
	{
		idx = Min(idx, self.m_size);

		n = self.m_size - idx;
	}

	auto stride = GetStride<OPTIMISATION>(self);
			
	auto newsize = self.m_size - n;

	if constexpr (IsObjectArray<ARRAY>::value)
	{
		REFLEX_LOOP_PTR(Reinterpret<Object*>(self.m_ptr) + idx, i, n)
		{
			Release(*i);
		}
	}

	auto ptr = Reinterpret<UInt8>(self.m_ptr);

	MemMove(ptr + ((idx + n) * stride), ptr + (idx * stride), (newsize - idx) * stride);

	if constexpr (IsObjectArray<ARRAY>::value)
	{
		Object * null = Cast<Object>(GetNull(self));

		REFLEX_LOOP_PTR(Reinterpret<Object*>(self.m_ptr) + newsize, i, n)
		{
			*i = null;

			null->RetainMt();
		}
	}

	UpdateSizeImpl(self, newsize);
}

void ShrinkImpl(Context & context)
{
	VM_POP(AbstractArray&,UInt32);

	auto & self = args.a;

	auto n = Min(args.b, self.m_size);

	UpdateSizeImpl(self, self.m_size - n);
}

void PopImpl(Context & context)
{
	VM_POP1(AbstractArray&);

	if (arg.m_size)
	{
		auto newsize = arg.m_size - 1;
		
	
		UpdateSizeImpl(arg, newsize);
	}
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void SetSizeImpl(Context & context)
{
	VM_POP(ARRAY&,UInt32);

	auto & self = args.a;

	auto newsize = args.b & kMaxSizeMask;

	auto top = self.m_size;

	if (newsize > top)
	{
		auto previous_capacity = self.m_capacity;

		if (newsize > previous_capacity)
		{
			ReallocateImpl<OPTIMISATION>(context, self, newsize);
		}

		WipeRegion<OPTIMISATION>(self, top, newsize - top);

		self.m_size = newsize;

		self.m_wrap = newsize;
	}
	else
	{
		UpdateSizeImpl(self, newsize);
	}
}

template <class ARRAY> void AppendImpl(Context & context)
{
	VM_POP(ARRAY&,ARRAY&);

	auto & self = args.a;

	auto & b = args.b;

	auto top = self.m_size;

	auto newsize = top + b.m_size;

	auto previous_capacity = self.m_capacity;

	if (newsize > previous_capacity)
	{
		ReallocateImpl<kArrayOptimisationNone>(context, self, newsize);
	}

	if constexpr (IsObjectArray<ARRAY>::value)
	{
		auto objects = Cast<Object*>(b.m_ptr);
		
		REFLEX_LOOP_PTR(Reinterpret<Object*>(self.m_ptr), i, b.m_size)
		{
			Reflex::Detail::SetReferenceCountedPointer(*i, *objects);
			
			objects++;
		}
	}
	//else if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	//{
	//	auto bytes = GetRawView<kArrayOptimisationValue32>(b);
	//	
	//	MemCopy(bytes.a, Reinterpret<UInt32>(self.m_ptr) + top, bytes.b);
	//}
	else
	{
		auto bytes = GetRawView<kArrayOptimisationNone>(b);

		MemCopy(bytes.data, Reinterpret<UInt8>(self.m_ptr) + (top * self.m_stride), bytes.size);
	}

	UpdateSizeImpl(self, newsize);
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void PushImpl(Context & context)
{
	REFLEX_INLINE_LOCAL(UInt, Resize)(Context & context, ARRAY & self)
	{
		auto top = self.m_size;

		auto newsize = self.m_size + 1;

		if (newsize > self.m_capacity) 
		{
			ReallocateImpl<OPTIMISATION>(context, self, newsize);
		}
		
		self.m_size = newsize;

		self.m_wrap = newsize;

		return top;
	}
	REFLEX_END

	if constexpr (IsObjectArray<ARRAY>::value)
	{
		VM_POP(ARRAY&,Object*);

		auto & self = args.a;

		auto top = Resize::Call(context, self);

		Reflex::Detail::SetReferenceCountedPointer(Reinterpret<Object*>(self.m_ptr)[top], args.b);
	}
	else if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		VM_POP(ARRAY&,Value32);

		auto & self = args.a;

		auto top = Resize::Call(context, self);

		Reinterpret<Value32>(self.m_ptr)[top] = args.b;
	}
	else
	{
		auto size = ReadFunctionData<UInt8>(context);

		auto args = Detail::Pop(context.stack, sizeof(Detail::Pointer) + size);

		auto & self = **Reinterpret<ValueArray*>(args);

		auto top = Resize::Call(context, self);

		MemCopy(args + sizeof(Detail::Pointer), Reinterpret<UInt8>(self.m_ptr) + (top * size), size);
	}
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> auto GetElementImpl(ARRAY & self, Index32 idx)
{
	idx = idx % self.m_wrap;

	if constexpr (IsObjectArray<ARRAY>::value)
	{
		return Reinterpret<Object*>(self.m_ptr) + idx;
	}
	else if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		return Reinterpret<Value32>(self.m_ptr) + idx;
	}
	else
	{
		return Reinterpret<UInt8>(self.m_ptr) + (idx * self.m_stride);
	}
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> auto opGetImpl(Context & context)
{
	VM_POP(ARRAY&,Index32);

	auto ptr = GetElementImpl<ARRAY,OPTIMISATION>(args.a, args.b);

	if constexpr (IsObjectArray<ARRAY>::value || OPTIMISATION == kArrayOptimisationValue32)
	{
		VM_RTN(*ptr);
	}
	else
	{
		auto size = ReadFunctionData<UInt8>(context);

		MemCopy(ptr, Extend(context.stack, size).data, size);
	}
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void opSetImpl(Context & context)
{
	if constexpr (IsObjectArray<ARRAY>::value)
	{
		VM_POP(ObjectArray&,Index32,Object*);

		auto ptr = GetElementImpl<ObjectArray,OPTIMISATION>(args.a, args.b);

		Reflex::Detail::SetReferenceCountedPointer(*ptr, args.c);
	}
	else if constexpr (OPTIMISATION == kArrayOptimisationValue32)
	{
		VM_POP(ValueArray&,Index32,Value32);
		
		auto ptr = GetElementImpl<ValueArray,kArrayOptimisationValue32>(args.a, args.b);

		*ptr = args.c;
	}
	else
	{
		auto size = ReadFunctionData<UInt8>(context);

		auto args = Detail::Pop(context.stack, sizeof(Detail::Pointer) + sizeof(UInt) + size);

		auto & self = **Reinterpret<ValueArray*>(args);

		auto idx = *Reinterpret<UInt32>(args + sizeof(Detail::Pointer));

		auto ptr = GetElementImpl<ValueArray,OPTIMISATION>(self, idx);

		auto pvalue = Reinterpret<void>(args + sizeof(Detail::Pointer) + sizeof(UInt));

		MemCopy(pvalue, ptr, size);
	}
}

template <class ARRAY, ArrayOptimisation OPTIMISATION = kArrayOptimisationNone> void JoinImpl(Context & context)
{
	auto n = GetNumberArgs(context);

	auto & [value_t, array_t] = ReadFunctionData<Pair<TypeRef>>(context);

	auto & stack = context.stack;

	auto args = Reinterpret<ARRAY*>(Detail::Pop(stack, n * sizeof(Detail::Pointer)));

	auto size = 0;

	REFLEX_LOOP_PTR(args, ptr, n) size += (**ptr).m_size;

	auto self = CreateEmptyArrayImpl<ARRAY>(context, array_t, value_t);

	auto data = RawAllocateImpl(context, *self, size);

	UpdateSizeImpl(*self, size);

	REFLEX_LOOP_PTR(args, ptr, n)
	{
		ARRAY & src = (**ptr);

		auto bytes = GetByteSize<OPTIMISATION>(src);

		MemCopy(src.m_ptr, data, bytes);

		data += bytes;
	}

	if constexpr (IsObjectArray<ARRAY>::value)
	{
		REFLEX_LOOP_PTR(Cast<Object*>(self->m_ptr), i, size)
		{
			Detail::Pointer pobject = *i;

			Retain(pobject);
		}
	}

	VM_RTN(self);
}

//void ValueArrayImpl::Append(const UInt8 * values, Index32 n)
//{
//	auto top = m_size;
//
//	auto newsize = m_size + n;
//
//	if (newsize > m_capacity) Allocate(newsize);
//
//	MemCopy(values, m_ptr + (top * m_stride), m_stride * n);
//
//	m_size = newsize;
//
//	m_wrap = newsize;
//}

REFLEX_INLINE void RegisterValueArray(Compiler::State & state, CString::View name, Type * array_t, TypeRef value_t, TypeRef iterator_t, const Data::Archive::View & data)
{
	auto bindings = state.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto int32_t = bindings->int32_t;

	auto archive_t = bindings->archive_t;


	Detail::SetTypeCtr(array_t, data, [](VM_CTR_PARAMS) -> Object &
	{
		auto value_t = Data::Unpack<TypeRef>(type->params);

		return *Cast<Object>(CreateEmptyArrayImpl<ValueArray>(context, type, value_t));
	});

	REFLEX_ASSERT(array_t->contextcopyfn[false] == Detail::kTrivialContextCopy);

	array_t->contextcopyfn[true] = &ArrayContextCopyMt;


	auto size_data = Data::Pack(value_t->size);


	UIntNative zeroinit = true;

	for (auto & i : value_t->params) zeroinit = zeroinit && (i == 0);


	Argument array_arg = { array_t, false };
	Argument value_arg = { value_t, false };
	Argument void_arg = { void_t, false };
	Argument index_arg = { int32_t, false };


	bindings->RegisterFunction(kGlobal, Compiler::opCreateArray, array_t, {value_t}, {value_t}, data, ExternalFunction::kFlagsVaradic, &CreateWithArgsImpl<ValueArray, kArrayOptimisationNone>);

	AddConstructor(bindings, array_t, { index_arg }, data, zeroinit ? &CreateWithSizeImpl<ValueArray,kArrayOptimisationZeroInit> : &CreateWithSizeImpl<ValueArray>);

	AddConstructor(bindings, array_t, { GetType<Allocation>(bindings) }, data, [](Context & context)
	{
		VM_POP1(Allocation&);

		auto & [value_t,array_t] = ReadFunctionData<Pair<TypeRef>>(context);

		auto self = Cast<AbstractArray>(array_t->ctr(context, array_t));

		self->SetData(arg);

		VM_RTN(*self);
	});

	AddMethod(bindings, "Copy", array_arg, { array_arg }, {}, data, &CopyImpl<ValueArray>);

	AddMethod(bindings, "Remove", void_arg, { array_arg, index_arg, index_arg }, &RemoveImpl<ValueArray>);

	AddMethod(bindings, "Remove", void_arg, { array_arg, index_arg }, [](Context & context)
	{
		VM::Detail::Push(context.stack, 1);

		RemoveImpl<ValueArray>(context);
	});

	AddMethod(bindings, "Wipe", void_arg, { array_arg }, zeroinit ? &WipeImpl<ValueArray,kArrayOptimisationZeroInit> : &WipeImpl<ValueArray>);

	AddMethod(bindings, "SetSize", void_arg, { array_arg, index_arg }, zeroinit ? &SetSizeImpl<ValueArray,kArrayOptimisationZeroInit> : &SetSizeImpl<ValueArray>);

	AddMethod(bindings, "Append", void_arg, { array_arg, array_arg }, &AppendImpl<ValueArray>);

	bool value32 = value_t->size == 4;

	if (value32)
	{
		AddMethod(bindings, "Fill", void_arg, { array_arg, value_arg }, &FillImpl<ValueArray,kArrayOptimisationValue32>);

		AddMethod(bindings, "Push", void_arg, { array_arg, value_arg }, &PushImpl<ValueArray,kArrayOptimisationValue32>);

		AddMethod(bindings, Compiler::opGet, value_arg, { array_arg, index_arg }, &opGetImpl<ValueArray,kArrayOptimisationValue32>);

		AddMethod(bindings, Compiler::opSet, void_arg, { array_arg, index_arg, value_arg }, &opSetImpl<ValueArray,kArrayOptimisationValue32>);

		AddMethod(bindings, "Search", bool_t, { array_arg, value_arg, ByRef(int32_t) }, &ValueArraySearch<kArrayOptimisationValue32>);

		AddMethod(bindings, "SearchRegion", bool_t, { array_arg, array_arg, ByRef(int32_t) }, &ValueArraySearchRegion<kArrayOptimisationValue32>);
	}
	else
	{
		AddMethod(bindings, "Fill", void_arg, { array_arg, value_arg }, {}, size_data, &FillImpl<ValueArray>);

		AddMethod(bindings, "Push", void_arg, { array_arg, value_arg }, {}, size_data, &PushImpl<ValueArray>);

		AddMethod(bindings, Compiler::opGet, value_arg, { array_arg, index_arg }, {}, size_data, &opGetImpl<ValueArray>);

		AddMethod(bindings, Compiler::opSet, void_arg, { array_arg, index_arg, value_arg }, {}, size_data, &opSetImpl<ValueArray>);

		AddMethod(bindings, "Search", bool_t, { array_arg, value_arg, ByRef(int32_t) }, {}, size_data, &ValueArraySearch<kArrayOptimisationNone>);

		AddMethod(bindings, "SearchRegion", bool_t, { array_arg, array_arg, ByRef(int32_t) }, &ValueArraySearchRegion<kArrayOptimisationNone>);
	}

	bindings->RegisterIntrinsic(kGlobal, Compiler::opNext, OPCODE(intrinsicIntegralArrayNext), value_t->size, 0, bool_t, { iterator_t, ByRef(value_t) });

	AddFunction(bindings, kGlobal, Compiler::opEqual, bool_t, { array_arg, array_arg }, &ValueArrayEqualImpl);

	AddFunction(bindings, kGlobal, Compiler::opInequal, bool_t, { array_arg, array_arg }, &ValueArrayInequalImpl);

	bindings->RegisterFunction(kGlobal, "Join", array_arg, { array_arg }, {}, data, ExternalFunction::kFlagsVaradic, &JoinImpl<ValueArray>);


	Detail::SetTypeCtr(iterator_t, {}, [](VM_CTR_PARAMS) -> Object &
	{
		return Detail::FinaliseObject(*REFLEX_CREATE(Detail::IntegralArrayIterator), type);
	});

	AddFunction(bindings, kGlobal, Compiler::opBegin, iterator_t, { array_t }, [](Context & context)
	{
		VM_POP1(ValueArray&);

		VM_RTN(REFLEX_CREATE(Detail::IntegralArrayIterator, arg.m_allocation, arg.m_ptr, Reinterpret<UInt8>(arg.m_ptr) + (arg.m_size * arg.m_stride)));
	});

	archive_t = archive_t ? archive_t : array_t;

	REFLEX_ASSERT(archive_t->type_id == K32("Array@UInt8"));

	AddMethod(bindings, VM::Serialize, void_t, { array_t, archive_t }, [](Context & context)
	{
		VM_POP(ValueArray&, ValueArray&);

		auto & stream = args.b;

		StoreValue(stream, args.a.m_size);

		VM::Append(stream, args.a.GetView<UInt8>());
	});

	AddMethod(bindings, VM::Deserialize, void_t, { array_t, archive_t }, [](Context & context)
	{
		VM_POP(ValueArray&,ValueArray&);

		auto & self = args.a;

		auto & stream = args.b;

		auto itemsize = self.m_stride;

		UInt32 n = RestoreValue<false,UInt32>(stream);

		UInt bytes = n * itemsize;

		if (bytes && stream.m_size >= bytes)
		{
			auto dest = RawAllocateImpl(context, self, n);

			MemCopy(stream.m_ptr, dest, bytes);

			UpdateSizeImpl(self, n);

			stream.Nudge(bytes);
		}
		else
		{
			self.Clear();
		}
	});
}

template <class ARRAY> Object & ObjectArrayCtr(VM_CTR_PARAMS)
{
	auto value_t = Data::Unpack<TypeRef>(type->params);

	auto self = CreateEmptyArrayImpl<ARRAY>(context, type, value_t);

	RawAllocateImpl(context, *self, 4);

	ConstructRegion<kArrayOptimisationNone>(*self, 0, 4);

	return *Cast<Object>(self);
}

REFLEX_INLINE void RegisterObjectArray(Compiler::State & state, CString::View name, Type * array_t, TypeRef value_t, TypeRef iterator_t, const Data::Archive::View & data)
{
	auto bindings = state.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto int32_t = bindings->int32_t;

	auto archive_t = bindings->archive_t;

	bool noncircular = value_t->flags.Check(Type::kFlagNonCircular);

	auto CrossContextCopyImpl = [](Context & context, Object & object, TypeRef array_t, bool mt) -> Object *
	{
		auto source = Cast<ObjectArray>(object)->GetRegion<Object>();

		auto copy = Cast<ObjectArray>(Detail::CreateObject(context, array_t));

		auto ptr = copy->Extend<Object>(context, source.size);

		auto value_t = Data::Unpack<TypeRef>(array_t->params);

		auto copyfn = value_t->contextcopyfn[mt];

		for (auto & i : source)
		{
			if (auto copied = copyfn(context, i, value_t, mt)) *ptr = copied;

			ptr++;
		}

		return copy.Adr();
	};

	if (value_t->flags.Check(Type::kFlagThreadsafe))
	{
		REFLEX_ASSERT(array_t->contextcopyfn[false] == Detail::kTrivialContextCopy);

		array_t->contextcopyfn[true] = &ArrayContextCopyMt;
	}
	else if (value_t->flags.Check(Type::kFlagNonCircular))
	{
		REFLEX_ASSERT(array_t->contextcopyfn[false] == Detail::kTrivialContextCopy);

		array_t->contextcopyfn[true] = CrossContextCopyImpl;
	}
	else
	{
		array_t->contextcopyfn[false] = CrossContextCopyImpl;

		array_t->contextcopyfn[true] = CrossContextCopyImpl;
	}


	Argument array_arg = { array_t, false };
	Argument value_arg = { value_t, false };
	Argument void_arg = { void_t, false };
	Argument index_arg = { int32_t, false };


	Type::Ctr ctr;

	ExternalFunctionPtr createwithargs, createwithsize, copy;

	if (noncircular)
	{
		ctr = &ObjectArrayCtr<ObjectArray>;

		createwithargs = &CreateWithArgsImpl<ObjectArray>;

		createwithsize = &CreateWithSizeImpl<ObjectArray>;

		copy = &CopyImpl<ObjectArray>;
	}
	else
	{
		ctr = &ObjectArrayCtr<CircularObjectArray>;

		createwithargs = &CreateWithArgsImpl<CircularObjectArray>;

		createwithsize = &CreateWithSizeImpl<CircularObjectArray>;

		copy = &CopyImpl<CircularObjectArray>;
	}

	Detail::SetTypeCtr(array_t, data, ctr);

	bindings->RegisterFunction(kGlobal, Compiler::opCreateArray, array_t, { value_t }, { value_t }, data, ExternalFunction::kFlagsVaradic, createwithargs);

	AddConstructor(bindings, array_t, { index_arg }, data, createwithsize);

	AddMethod(bindings, "Copy", array_arg, { array_arg }, {}, data, copy);

	AddMethod(bindings, "Remove", void_arg, { array_arg, index_arg, index_arg }, &RemoveImpl<ObjectArray>);

	AddMethod(bindings, "Remove", void_arg, { array_arg, index_arg }, [](Context & context)
	{
		VM::Detail::Push(context.stack, 1);

		RemoveImpl<ObjectArray>(context);
	});

	AddMethod(bindings, "Wipe", void_arg, { array_arg }, &WipeImpl<ObjectArray>);

	AddMethod(bindings, "Fill", void_arg, { array_arg, value_arg }, &FillImpl<ObjectArray>);

	AddMethod(bindings, "SetSize", void_arg, { array_arg, index_arg }, &SetSizeImpl<ObjectArray>);

	AddMethod(bindings, "Append", void_arg, { array_arg, array_arg }, &AppendImpl<ObjectArray>);

	AddMethod(bindings, "Push", void_arg, { array_arg, value_arg }, &PushImpl<ObjectArray>);

	AddMethod(bindings, Compiler::opGet, value_arg, { array_arg, index_arg }, &opGetImpl<ObjectArray>);

	AddMethod(bindings, Compiler::opSet, void_arg, { array_arg, index_arg, value_arg }, &opSetImpl<ObjectArray>);

	bindings->RegisterFunction(kGlobal, "Join", array_arg, { array_arg }, {}, data, ExternalFunction::kFlagsVaradic, &JoinImpl<ObjectArray>);

	Detail::SetTypeCtr(iterator_t, {}, [](VM_CTR_PARAMS) -> Object &
	{
		return FinaliseObject(*REFLEX_CREATE(Detail::ObjectArrayIterator), type);
	});

	AddFunction(bindings, kGlobal, Compiler::opBegin, iterator_t, { array_t }, [](Context & context)
	{
		auto & stack = context.stack;

		auto & array = Detail::Pop<ObjectArray&>(stack);

		auto ptr = Cast<Reference<Object>>(array.m_ptr);

		VM_RTN(REFLEX_CREATE(Detail::ObjectArrayIterator, array.null, array.m_allocation, ptr, ptr + array.m_size));
	});

	bindings->RegisterIntrinsic(kGlobal, Compiler::opNext, OPCODE(intrinsicObjectArrayNext), !value_t->flags.Check(Type::kFlagThreadsafe), 0, bool_t, { iterator_t, ByRef(value_t) });

	if (auto pack = GetStore(state, value_t))
	{
		AddMethod(bindings, VM::Serialize, void_t, { array_t, archive_t }, {}, Data::Pack(pack), [](Context & context)
		{
			auto & pack = *ReadFunctionData<Function*>(context);

			VM_POP(ObjectArray&,ValueArray&);

			auto src = args.a.GetRegion<Object>();

			UInt32 n = src.size;

			StoreValue(args.b, n);

			REFLEX_LOOP_PTR(src.data, ptr, n) Dispatch(context, pack, { ptr->Adr(), &args.b });
		});
	}

	if (auto unpack = GetRestore(state, value_t))
	{
		AddMethod(bindings, VM::Deserialize, void_t, { array_t, archive_t }, {}, Data::Pack(MakeTuple(unpack, value_t)), [](Context & context)
		{
			auto [unpack,value_t] = ReadFunctionData<Pair<Function*,TypeRef>>(context);

			VM_POP(ObjectArray&,ValueArray&);

			auto ctr = value_t->ctr;

			UInt32 n = RestoreValue<false,UInt32>(args.b);

			auto region = args.a.Extend<Object>(context, n);

			REFLEX_LOOP(idx, n)
			{
				auto & object = ctr(context, value_t);

				region[idx] = object;	//retained

				Dispatch(context, *unpack, { &object, &args.b });
			}
		});
	}
}

TypeRef InstantiateArray(Compiler::State & state, const Compiler::State::ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs)
{
	auto bindings = state.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto int32_t = bindings->int32_t;

	auto value_t = targs.GetFirst();

	bool noncircular = value_t->flags.Check(Type::kFlagNonCircular);

	auto array_object_t = Detail::AcquireObjectType(state, REFLEX_OBJECT_TYPE(Object), kGlobal, name);

	auto array_t = Detail::CreateObjectType(bindings, array_object_t, kGlobal, name, noncircular, false);

	Detail::SetTypeFlag(array_t, Type::kFlagExplicitNullable, false);

	auto members = Extend(array_t->members, 2);

	members[0] = { K32("allocation"), MakeMember(bindings->object_t, REFLEX_OFFSETOF(AbstractArray, m_allocation), true) };

	members[1] = { K32("size"), MakeMember(int32_t, REFLEX_OFFSETOF(AbstractArray, m_size), true) };
	
	auto iterator_name = AcquireStaticString(state, Detail::MakeTemplateName(state, "ArrayIterator", { value_t }));

	auto iterator_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(state, REFLEX_OBJECT_TYPE(Object), kGlobal, iterator_name), kGlobal, iterator_name, noncircular, false);

	auto data_t = MakeTuple(value_t, array_t);	//XCODE_WORKAROUND

	auto data = Data::Pack(data_t);

	Argument void_arg = { void_t, false };

	Argument array_arg = { array_t, false };

	Argument index_arg = { int32_t, false };

	AddMethod(bindings, "Share", array_arg, { array_arg }, {}, data, &ShareImpl);

	AddMethod(bindings, "Clear", void_arg, { array_arg }, &ClearImpl);

	AddMethod(bindings, "Nudge", void_arg, { array_arg, index_arg }, &NudgeImpl);

	AddMethod(bindings, "Shrink", void_arg, { array_arg, index_arg }, &ShrinkImpl);

	AddMethod(bindings, "Pop", void_arg, { array_arg }, &PopImpl);

	if (value_t->IsObject())
	{
		RegisterObjectArray(state, name, array_t, value_t, iterator_t, data);
	}
	else
	{
		RegisterValueArray(state, name, array_t, value_t, iterator_t, data);
	}

	AddFunction(bindings, kGlobal, Compiler::opCast, bool_t, { array_t }, { bool_t }, {}, [](Context & context)
	{
		VM_RTN(True(Detail::Pop<AbstractArray&>(context.stack).m_size));
	});

	return array_t;
}

struct NullAllocation : public ValueAllocation
{
	NullAllocation()
		: ValueAllocation(4, 1)
	{
	}

	UInt64 data = 0;
};

NullAllocation g_null_allocation;

REFLEX_END_INTERNAL




//
//

Reflex::TRef <Reflex::VM::Allocation> Reflex::VM::Allocation::Create(Context & context, TypeRef type, UInt size)
{
	Allocation * allocation;
	
	if (type->IsObject())
	{
		allocation = Reflex::Detail::Constructor<ObjectAllocation>::CreateVariableSize(g_default_allocator, size * sizeof(Detail::Pointer), context, size);
	}
	else
	{
		auto stride = type->size;

		allocation = Reflex::Detail::Constructor<ValueAllocation>::CreateVariableSize(g_default_allocator, size * stride, stride, size);
	}

	return allocation;
}

REFLEX_INLINE Reflex::VM::Allocation::Allocation(UInt size)
	: size(size),
	reserved(0)
{
	REFLEX_ASSERT(size);
}

void Reflex::VM::AbstractArray::SetDataExternal(Object & allocation, void * ptr, UInt size)
{
	if (size)
	{
		m_allocation = allocation;

		m_ptr = ptr;

		m_capacity = size;

		UpdateSizeImpl(*this, size);
	}
}

Reflex::VM::AbstractArray::AbstractArray(TypeRef type, UInt value_size, Object & allocation, void * data)
	: m_allocation(allocation),
	m_ptr(Cast<UInt8>(data)),
	m_stride(value_size),
	m_capacity(0),
	m_size(0),
	m_wrap(1)
{
	Detail::FinaliseObject(*this, type);
}

Reflex::VM::ValueArray::ValueArray(TypeRef array_t, TypeRef value_t)
	: AbstractArray(array_t, value_t->size, REFLEX_NULL(Object), ValueArray::null)
{
	MemCopy(value_t->params.GetData(), ValueArray::null, m_stride);
}

bool Reflex::VM::ValueArray::SetData(Allocation & allocation)
{
	if (DynamicCast<ValueAllocation>(allocation))
	{
		if (ToUIntNative(allocation.reserved) == m_stride)
		{
			AbstractArray::SetDataExternal(allocation, allocation.data, allocation.size);

			return true;
		}
	}

	return false;
}

Reflex::TRef <Reflex::VM::Allocation> Reflex::VM::ValueArray::CreateAllocation(Context & context, UInt size)
{
	return Reflex::Detail::Constructor<ValueAllocation>::CreateVariableSize(g_default_allocator, size * m_stride, m_stride, size);
}

void * Reflex::VM::ValueArray::GetNull()
{
	return RemoveConst(null);
}

void * Reflex::VM::ValueArray::Extend(UInt n)
{
	auto top = m_size;

	auto newsize = top + n;

	auto previous_capacity = m_capacity;

	if (newsize > previous_capacity)
	{
		ReallocateImpl<kArrayOptimisationNone>(REFLEX_NULL(Context), *this, newsize);
	}

	//WipeRegion<kArrayOptimisationNone>(*this, top, n);

	UpdateSizeImpl(*this, newsize);

	return Cast<UInt8>(m_ptr) + (top * m_stride);
}

Reflex::VM::ObjectArray::ObjectArray(Context & context, TypeRef array_t, TypeRef value_t)
	: AbstractArray(array_t, sizeof(Detail::Pointer), REFLEX_NULL(Object), &VM::Detail::GetNull(context, value_t)),
	null(Reinterpret<Object>(AbstractArray::m_ptr))
{
}

Reflex::TRef <Reflex::VM::Allocation> Reflex::VM::ObjectArray::CreateAllocation(Context & context, UInt size)
{
	return Reflex::Detail::Constructor<ObjectAllocation>::CreateVariableSize(g_default_allocator, size * m_stride, context, size);
}

void * Reflex::VM::ObjectArray::GetNull()
{
	return null.Adr();
}

bool Reflex::VM::ObjectArray::SetData(Allocation & allocation)
{
	if (DynamicCast<ObjectAllocation>(allocation))
	{
		AbstractArray::SetDataExternal(allocation, allocation.data, allocation.size);

		return true;
	}

	return false;
}

void Reflex::VM::ObjectArray::OnReleaseData()
{
	m_allocation.Clear();

	m_ptr = null.Adr();

	m_size = 0;

	m_wrap = 1;
}

Reflex::Object ** Reflex::VM::ObjectArray::Extend(Context & context, UInt n)
{
	auto top = m_size;

	auto newsize = top + n;

	auto previous_capacity = m_capacity;

	if (newsize > previous_capacity)
	{
		ReallocateImpl<kArrayOptimisationNone>(context, *this, newsize);
	}

	WipeRegion<kArrayOptimisationNone>(*this, top, n);

	UpdateSizeImpl(*this, newsize);

	return Cast<Object*>(m_ptr) + top;
}

REFLEX_SET_TRAIT(Reflex::VM::ValueArray, IsNonCircular);
REFLEX_SET_TRAIT(Reflex::VM::ValueAllocation, IsNonCircular);
REFLEX_SET_TRAIT(Reflex::VM::ValueAllocation, IsThreadSafe);




//
//VM bindings

void Reflex::VM::BindArray(Compiler::State & state)
{
	state.RegisterTemplateType(kGlobal, "Array", 1, false, {}, &InstantiateArray);

	constexpr auto InstantiateCreateArray = [](Compiler::State & state, Key32 ns, const CString::View & name, const ArrayView <Argument> &targs, const ArrayView <Argument> &args)
	{
		state.InstantiateTemplateType(kArray, { targs.GetFirst().type });

		return true;
	};

	VM_TBIND_VARADIC(InstantiateCreateArray, state, kGlobal, Compiler::opCreateArray, 1, 0);
}

Reflex::VM::TypeRef Reflex::VM::Detail::BindArrayUInt8(Bindings & bindings)
{
	struct CompilerStateProxy : public Compiler::State
	{
		using Compiler::State::State;

		virtual bool Instantiate(const Module & module) override { return false; }

		virtual CString::View RegisterStaticString(Key32 id, CString && string) override 
		{
			return GetStaticString(id);
		}

		virtual CString::View GetStaticString(Key32 id, bool assume = false) const override
		{ 
			switch (id.value)
			{
			case Reflex::kHashSeed:
				return "";

			case K32("Array@UInt8"):
				return "Array@UInt8";

			case K32("ArrayIterator@UInt8"):
				return "ArrayIterator@UInt8";

			default:
				REFLEX_ASSERT(false);
				return "";
			}
		}

		virtual void RegisterResourceType(Key32 id, TypeRef type, File::ResourcePool::Ctr ctr, const WString::View & ext, ConstTRef <Data::PropertySet> options) override {}

		virtual Symbol RegisterConstant(TypeRef typeref, Key32 ns, const CString::View & name, const Data::Archive::View & value) override { return { ns, name }; }

		virtual void EnumerateConstants(const Reflex::Function <void(const Constant&)> & callback) const override {}

		virtual Symbol RegisterTemplateType(Key32 ns, const CString::View & name, UInt ntarg, bool varadic, const Data::Archive::View & clientdata, TemplateType::Instantiator callback) override { return { ns, name }; }

		virtual void EnumerateTemplateTypes(const Reflex::Function <void(const TemplateType&)> & callback) const override {}

		virtual Symbol RegisterTemplateFunction(Key32 ns, const CString::View & name, UInt32 ntarg, const ArrayView <Argument> & args, TemplateFunction::Instantiator callback, UInt8 flags = 0) override { return { ns, name }; }

		virtual void EnumerateTemplateFunctions(const Reflex::Function <void(const TemplateFunction&)> & callback) const override {}

		virtual TypeRef InstantiateTemplateType(Symbol symbol, const ArrayView <TypeRef> & targs) override { return 0; }

		virtual const InstantiatedTemplateType * GetInstantiatedTemplateType(TypeRef type) const override { return 0; }

		virtual void RegisterTypedef(Symbol symbol, TypeRef type) override {}

		virtual void EnumerateScriptFunctions(Symbol symbol, const Reflex::Function <void(const ScriptFunction&)> & callback) const override {}
	};
	
	TypeRef array_t;

	Reflex::Detail::SilentReference <Bindings> retain(bindings);

	{
		CompilerStateProxy state(bindings);

		auto allocation_t = VM::RegisterObject<Allocation>(bindings, kGlobal, "Allocation");

		auto valueallocation_t = VM::RegisterObject<ValueAllocation>(bindings, kGlobal, "ValueAllocation");

		allocation_t->null = [](VM_CTR_PARAMS) -> Object &
		{
			return g_null_allocation;
		};

		valueallocation_t->null = [](VM_CTR_PARAMS) -> Object &
		{
			return g_null_allocation;
		};

		REFLEX_ASSERT(valueallocation_t->contextcopyfn[true] == Detail::kTrivialContextCopy);

		AddConstructor(bindings, valueallocation_t, { bindings.key32_t, bindings.int32_t }, [](Context & context)
		{
			VM_POP(TypeID, UInt32);

			if (auto value_t = context.program->bindings->GetTypeByRTTID(args.a))
			{
				if (!value_t->IsObject())
				{
					auto stride = value_t->size;

					VM_RTN(Reflex::Detail::Constructor<ValueAllocation>::CreateVariableSize(g_default_allocator, args.b * stride, stride, args.b));
				}
			}
			else
			{
				VM_RTN(&g_null_allocation);
			}
		});

		array_t = InstantiateArray(state, {}, kGlobal, "Array@UInt8", { bindings.uint8_t });
	}

	return array_t;
}
