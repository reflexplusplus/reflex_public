#include "../../../include/reflex/core/list.h"
#include "../../../include/reflex/core/tuple.h"
//#include "../../../include/reflex/core/function/member.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::Detail)

template <class TYPE> void AssertAlignment(const void * ptr)
{
#if REFLEX_DEBUG
	UIntNative adr = (UIntNative)ptr;

	REFLEX_ASSERT((adr & (alignof(TYPE) - 1)) == 0);
#endif
}

constexpr Pair <FunctionPointer<void(const Object&)>> kRetainRelease[2] = 
{
	{
		[](const Object & object)
		{
			object.RetainMt();
		},
		[](const Object & object)
		{
			object.ReleaseMt();
		}
	},
	{
		[](const Object & object)
		{
			object.RetainSt();
		},
		[](const Object & object)
		{
			object.ReleaseSt();
		}
	}
};

template <bool REVERSE, class RTN> REFLEX_INLINE bool SafeIterateImpl(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <RTN(void*, Object&)> callback)
{
	typedef Object * Pointer;

	constexpr UInt kWrap = (alignof(Pointer) - 1);

	typedef Item <Object> Item;


	ArrayRegion <Pointer> region = { 0, list.GetNumItem() };

	auto allocated = UInt((region.size * sizeof(Pointer)) + alignof(Pointer));

	auto bytes = Reinterpret<UInt8>(REFLEX_STACKALLOC(allocated));

	auto current = ToUIntNative(bytes);

	auto shift = (alignof(Pointer) - current) & kWrap;

	bytes += shift;

	AssertAlignment<Pointer>(bytes);

	REFLEX_ASSERT((bytes + (sizeof(Pointer) * region.size)) <= (bytes + allocated));

	region.data = Reinterpret<Pointer>(bytes);


	item_offset -= Item::GetOffset();

	auto [retain, release] = kRetainRelease[st];

	auto itr = list.GetFirst();

	for (auto & i : region)
	{
		i = itr;

		retain(*itr);

		auto pitem = Reinterpret<Item>(Reinterpret<UInt8>(i) + item_offset);

		itr = pitem->GetNext();
	}

	bool rtn = false;

	if constexpr (REVERSE)
	{
		REFLEX_RFOREACH(i, region)
		{
			if constexpr (IsType<RTN, void>::value)
			{
				callback(client, *i);
			}
			else
			{
				if (callback(client, *i))
				{
					rtn = true;

					break;
				}
			}
		}
	}
	else
	{
		for (auto & i : region)
		{
			if constexpr (IsType<RTN, void>::value)
			{
				callback(client, *i);
			}
			else
			{
				if (callback(client, *i))
				{
					rtn = true;

					break;
				}
			}
		}
	}

	REFLEX_RFOREACH(i, region)
	{
		release(*i);
	}

	return rtn;
}

REFLEX_END_INTERNAL

void Reflex::Detail::SafeIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <void(void*, Object&)> callback)
{
	SafeIterateImpl<false,void>(list, item_offset, st, client, callback);
}

bool Reflex::Detail::SafeIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <bool(void*, Object&)> callback)
{
	return SafeIterateImpl<false,bool>(list, item_offset, st, client, callback);
}

void Reflex::Detail::SafeReverseIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <void(void*, Object&)> callback)
{
	SafeIterateImpl<true,void>(list, item_offset, st, client, callback);
}

bool Reflex::Detail::SafeReverseIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <bool(void*, Object&)> callback)
{
	return SafeIterateImpl<true,bool>(list, item_offset, st, client, callback);
}
