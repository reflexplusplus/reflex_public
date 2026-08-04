REFLEX_SET_TRAIT(Reflex::System::Process, IsThreadSafe);

REFLEX_BEGIN_INTERNAL(Reflex::VM)

void BindProcess(Bindings & bindings)
{
	auto thread_t = RegisterObject<System::Thread>(bindings, kSystem, "Thread");

	Detail::UseGlobalNull<System::Thread>(thread_t);

	Detail::BindObjectMethod<&System::Thread::Completed>(bindings, thread_t, "Completed");


	auto task_t = RegisterObject<System::Process>(bindings, kSystem, "Process");

	auto string_array_t = bindings.GetTypeByRTTID(VM::ArrayOfStringTarg::kRTTID);

	AddConstructor(bindings, task_t, { bindings.string_t, string_array_t }, [](Context & context)
	{
		VM_POP(String&,ArrayOfNonCircularObjects&);

		Array <WString> arguments;
		
		arguments.Allocate(args.b.m_size);

		auto data = args.b.GetView<String>().data;

		REFLEX_LOOP(idx, args.b.m_size) arguments.Push<kAllocateNone>(data[idx]->GetView());

		VM_RTN(System::Process::Create(args.a.GetView(), arguments).Adr());
	});

	Detail::BindObjectMethod<&System::Process::Terminate>(bindings, task_t, "Terminate");
}

REFLEX_END_INTERNAL
