
REFLEX_BEGIN_INTERNAL(Reflex::VM)

inline const Key32 kTupleMembers[] = { "a","b","c","d","e","f","g","h" };

void RegisterTuple(Compiler::State & cstate)
{
	char t[2] = { 'a', 0 };

	REFLEX_LOOP(idx, 8)
	{
		AcquireStaticString(cstate, t);

		t[0]++;
	}

	cstate.RegisterTemplateType(kGlobal, "Tuple", 8, true, {}, [](Compiler::State & cstate, const Compiler::State::ClientData, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs)
	{
		auto pname = kTupleMembers;

		Detail::Variables members;

		Int16 address = 0;

		bool object = false;

		REFLEX_FOREACH(type, targs)
		{
			object = object || type->IsObject();

			members.Push({ *pname++, MakeMember(type, address, false) });

			address += type->size;
		}

		typedef Detail::LayoutTemplateImpl <true> LayoutTemplateX;

		LayoutTemplateX layout;

		address = 0;

		for (auto & i : members)
		{
			auto type = i.b.type;

			if (type->IsObject())
			{
				auto ctr = type->ctr;

				Type::Ctr t = ctr ? ctr : type->null;

				if (!t) return TypeRef(0);

				layout.objects.Push({ UInt16(address), type, t });

				MemCopy(&REFLEX_NULL(Object), Extend(layout.init_state, sizeof(void *)).data, sizeof(void *));
			}
			else
			{
				layout.init_state.Append(type->params);
			}

			address += type->size;
		}

		layout.init_state.SetSize(RoundUpPow2(address));

		if (object)
		{
			return Detail::ScriptObject::RegisterType(cstate, {}, ns, name, std::move(members), &layout);
		}
		else
		{
			Key32 type_id = Detail::MakeNamespacedSymbol(cstate, ns, name);

			auto type = Detail::CreateValueType(cstate.bindings, type_id.value, ns, name, layout.init_state);

			type->members = std::move(members);

			return TypeRef(type);
		}
	});
}

REFLEX_END_INTERNAL

