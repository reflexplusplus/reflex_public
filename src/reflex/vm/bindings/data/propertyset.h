#pragma once




REFLEX_BEGIN_INTERNAL(Reflex::VM)

struct PropertySet :
	public Data::PropertySet,
	public VM::Detail::Circular
{
	PropertySet(VM::Context & context)
		: VM::Detail::Circular(context, *this)
	{
	}

	static Object * CrossContextCopy(VM::Context & context, Object & source, VM::TypeRef propertyset_t, bool mt)
	{
		auto & dynamic = propertyset_t->ctr(context, propertyset_t);

		for (auto & i : Cast<Data::PropertySet>(source)->Iterate())
		{
			if (auto copy = VM::Detail::CrossContextCopy(i.value, context, mt))
			{
				dynamic.SetProperty(i.key, *copy);
			}
		}

		return &dynamic;
	};
};

struct ReadOnlyPropertySet : public Data::PropertySet
{
	ReadOnlyPropertySet(Data::PropertySet && in)
	{
		for (auto & i : in.Iterate())
		{
			Data::PropertySet::OnSetProperty(i.key, i.value);
		}
	}

	void OnUnsetProperty(Address adr) override 
	{
		output.Error("can not write to read-only PropertySet");
	}

	void OnSetProperty(Address adr, Object & object) override 
	{
		output.Error("can not write to read-only PropertySet");

		AutoRelease(object); 
	}
};

REFLEX_END_INTERNAL
