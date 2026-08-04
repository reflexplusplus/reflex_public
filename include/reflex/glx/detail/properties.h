#pragma once

#include "../[require].h"




//
//boxlayer

REFLEX_NS(Reflex::GLX::Detail)

struct GenericLayer;

enum PropertyBindStage : UInt8
{
	kBindStageNone = 0,
	kBindStageBuildLayout = 1,
	kBindStageAccommodate = 2,
	kBindStageRealign = 4,
	kBindStageRedraw = 8
};

struct GenericPropertiesSchema : public Reflex::Object
{
	REFLEX_OBJECT(GenericPropertiesSchema,Object);

	GenericPropertiesSchema()
	{
		RemoveConst(defs).Allocate(8);
	}

	~GenericPropertiesSchema()
	{
		state_ctr_dtr.b(default_state);
	}

	struct Item
	{
		using Setter = FunctionPointer <void(const Object & source_property, void * dest, UInt64 data, UInt16 stylesheet_flags)>;

		UInt16 offset;
		UInt8 flags;
		PropertyBindStage bindable;
		const char * tname;
		Address address;
		UInt64 data;
		Setter apply;
	};

	void RegisterProperty(const Item & item)
	{
		RemoveConst(defs).Push(item);
	}

	Item * RegisterProperties(UInt n)
	{
		return Extend(RemoveConst(defs), n).data;
	}

	void Pop()	//workaround for Gradient 
	{
		RemoveConst(defs).Pop();
	}
	
	void FinaliseBindings()
	{
		REFLEX_IF_DEBUG(Map <Address,bool> map);

		auto & bindable = RemoveConst(GenericPropertiesSchema::bindable);

		bindable.Allocate(4);

		for (auto & i : defs)
		{
			if (i.bindable)
			{
				bindable.Push(&i);
			}

			REFLEX_ASSERT(SetFiltered(map.Acquire(i.address), true));
		}


	}

	Key32 uid;

	const Array <Item> defs;

	const Array <const Item*> bindable;

	FunctionPointer <void(GenericLayer&, const void*, void*, void*/*VTable&*/)> oninit;

	UInt16 state_size;

	UInt8 scratch_offset;

	bool complex;

	Tuple < FunctionPointer <void(void*, const void*)>, FunctionPointer <void(void*)> > state_ctr_dtr;

	UInt8 default_state alignas(16)[264];	//MAKE VARIABLE SIZE
};

REFLEX_END