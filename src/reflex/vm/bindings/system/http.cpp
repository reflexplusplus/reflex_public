
REFLEX_BEGIN_INTERNAL(Reflex::VM)

void BindHttpAndSleep(VM::Compiler::State & cstate, UInt8 context, TypeRef pair_string_t, TypeRef array_pair_string_t)
{
	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto string_t = bindings->string_t;


	auto http_t = RegisterObject<System::HttpConnection>(bindings, kSystem, "HttpConnection");

	VM::Detail::UseGlobalNull<System::HttpConnection>(http_t);


	AddConstructor(bindings, http_t, { bool_t, string_t }, [](Context & context)
	{
		VM_POP(bool,String&);

		VM_RTN(System::HttpConnection::Create(args.a, ToCString(args.b.GetView())).Adr());
	});



	//access

	struct Callbacks
	{
		Callbacks(VM::Context & context, Pair <VM::Detail::FnObject&> callbacks)
			: m_headers(callbacks.a),
			m_body(callbacks.b),
			m_buffer(REFLEX_CREATE(Data::ArchiveObject, kMaxUInt16)),
			m_vmbuffer(CreateByteArray({}))
		{
		}

		void OnReceiveHeader(const CString::View & key, const CString::View & value)
		{
			auto k = AutoRelease(String::Create(key));

			auto v = AutoRelease(String::Create(value));

			m_headers->context.Call(m_headers, *k, *v);
		}

		bool OnReceiveData(const ArrayView <UInt8> & bytes)
		{
			if (bytes)
			{
				auto & buffer = m_buffer->value;
				
				buffer = bytes;
				
				m_vmbuffer->SetData<UInt8>(m_buffer, { buffer.GetData(), bytes.size });

				return VM::CallReturningValue<bool>(m_body->context, *m_body, *m_vmbuffer);
			}

			return true;
		}

		Reference <VM::Detail::FnObject> m_headers, m_body;

		Reference <Data::ArchiveObject> m_buffer;

		Reference <VM::ValueArray> m_vmbuffer;
	};


	if (context & (VM::kContextFlagMainBg | VM::kContextFlagUiBg | kContextFlagRtBg | kContextFlagCustomBg))
	{
		auto int32_t = bindings->int32_t;

		auto archive_t = bindings->archive_t;

		auto headers_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, string_t, string_t });

		auto body_t = cstate.InstantiateTemplateType(VM::kFn, { bool_t, archive_t });

		auto data = MakeTuple(pair_string_t->members[0].b.address, pair_string_t->members[1].b.address);

		AddMethod(bindings, "Request", int32_t, { http_t, string_t, string_t, array_pair_string_t, archive_t, headers_t, body_t }, {}, Data::Pack(data), [](Context & context)
		{
			auto data = ReadFunctionData<Pair<UInt16>>(context);

			VM_POP(System::HttpConnection&,String&,String&,ArrayOfNonCircularObjects&,ValueArray&,Pair<VM::Detail::FnObject&>);

			auto & self = args.a;

			auto & method = args.b;
			auto & resource = args.c;
			auto & headers_in = args.d;

			Callbacks client(context, args.f);

			auto & body = args.e;

			Array <Pair<CString>> headers;

			headers.Allocate(headers_in.m_size);

			for (auto & i : headers_in.GetView<Object>())
			{
				auto & k = VM::GetMemberAtAdr<String>(i, data.a);

				auto & v = VM::GetMemberAtAdr<String>(i, data.b);

				headers.Push({ ToCString(k.GetView()), ToCString(v.GetView()) });
			}

			Int32 response = self.Request(ToCString(method.GetView()), ToCString(resource.GetView()), headers, body.GetView<UInt8>(), BindMethod(client, &Callbacks::OnReceiveHeader, _P1, _P2), BindMethod(client, &Callbacks::OnReceiveData, _P1));

			VM_RTN(response);
		});

		AddFunction(bindings, kSystem, "Sleep", void_t, { int32_t }, [](Context & context)
		{
			VM_POP1(Int32);

			System::SuspendThread(Min<UInt32>(arg, 1000));
		});
	}
}

REFLEX_END_INTERNAL
