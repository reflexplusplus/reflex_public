#include "../library.h"
#include "program/clock.h"
#include "../compiler/compilerimpl.h"




//TODO

REFLEX_BEGIN_INTERNAL(Reflex::VM)

struct ThreadImpl : public System::Thread
{
	REFLEX_OBJECT(ThreadImpl, System::Thread);

	void RunBG(const ScriptFunction * fn)
	{
		ScopeTimer time(output, "thread");

		auto & context = *m_context;

		Context::Scope scope(context);

		context.InitialiseGlobals(REFLEX_NULL(Object));

		rtn = CallReturningObject<Reflex::Object>(context, *fn, *m_input);
	}

	virtual bool Completed() const override { return True(rtn); }

	virtual void Wait() override {}


	Reference <Context> m_context;

	Reference <Object> m_input;

	Reference <Object> rtn;

	Reference <System::Thread> m_thread;
};

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gCoreProgram("Program", { &gSystem }, kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object &)
{
	auto bindings = cstate.bindings;


	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;

	auto object_t = bindings->object_t;


	auto program_t = RegisterObject<Program>(bindings, kGlobal, "Program");

	cstate.RegisterResourceType(K32("program"), program_t, [](const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream)
	{
		auto pcompilestate = Detail::CompilerImpl::StateImpl::ScopeOf::GetCurrent();

		auto & lock = *pcompilestate->filesystemlock;

		auto main = pcompilestate->m_target;
			
		auto bg = pcompilestate->m_compiler->Compile(lock, ctx.path, kContextFlagMainBg, Null<Object>(), {});

		for (auto & i : bg->sources)
		{
			REFLEX_FOREACH(current, main->sources)
			{
				if (i.address == current.address) goto Skip;
			}

			Detail::AddSource(main, i.path, i.address.type_id, i.object);
			
			REFLEX_MARKER(Skip);
		}
		
		return TRef<Reflex::Object>(bg);
	}, L"c");

	AddConstructor(bindings, program_t, { bindings->string_t }, [](Context & context)
	{
		UInt8 context_flags = context.program->bindings->context_flags;

		context_flags = context_flags < 16 ? context_flags << 4 : context_flags;

		REFLEX_ASSERT(context_flags & (kContextFlagMainBg | kContextFlagUiBg | kContextFlagRtBg | kContextFlagCustomBg));

		VM_POP1(String&);

		if (auto resourcepool = context.QueryProperty<File::ResourcePool>(kNullKey))
		{
			File::ResourcePool::Lock lock(*resourcepool);

			auto compiler = AutoRelease(New<Compiler>());

			auto & base = context.program->sources.GetFirst().pathview;

			auto fullpath = File::ResolveIncludePath(File::SplitFilename(base).a, arg.GetView());

			auto target = compiler->Compile(lock, fullpath, context_flags);

			VM_RTN(target.Adr());
		}
		else
		{
			output.Error("Program", "context has no resourcepool");

			VM_RTN(REFLEX_NULL(Program));
		}
	});

	Detail::UseGlobalNull<Program>(program_t);


	auto context_t = RegisterObject<Context>(bindings, kGlobal, "Context");

	Detail::UseGlobalNull<Context>(context_t);

	AddConstructor(bindings, context_t, { program_t }, [](Context & context)
	{
		auto & program = Detail::Pop<Program&>(context.stack);

		auto self = Context::Create(context.GetContextID());

		self->Initialise(program);

		VM_RTN(self.Adr());
	});


	auto thread_t = RegisterObject<ThreadImpl>(bindings, kGlobal, "Thread");

	thread_t->members.Push(VM_BIND_MEMBER(ThreadImpl, rtn, cstate, object_t));

	auto array_key32_t = cstate.InstantiateTemplateType(kArray, { key32_t });

	auto array_object_t = cstate.InstantiateTemplateType(kArray, { object_t });

	AddConstructor(bindings, thread_t, { program_t, key32_t, key32_t, key32_t, array_key32_t, array_object_t }, [](Context & context)
	{
		VM_POP(Program&, Key32, Key32, TypeID, ValueArray&, ArrayOfCircularObjects&);

		auto rtn = REFLEX_CREATE(ThreadImpl);

		auto fnargs = args.e.GetView<TypeID>();

		auto object_args = args.f.GetView<Object>();

		if (fnargs.size == 1 && fnargs.size == object_args.size)
		{
			auto & program = args.a;

			for (auto & i : program.GetScriptFunctions({ args.b, args.c }))
			{
				auto & fn = i.value;

				auto rtn_t = fn.rtn.type;

				if (fn.instructions && rtn_t->IsObject() && fn.args.GetSize() == fnargs.size && rtn_t->type_id == args.d)
				{
					auto p0_t = fn.args[0].type;

					if (p0_t->IsObject() && p0_t->type_id == fnargs.GetFirst())
					{
						if (Reflex::Detail::CheckObjectType(object_args.GetFirst(), p0_t->object_t))
						{
							if ((rtn->m_context = Context::Create(Reflex::Detail::AcquireContextID())))
							{
								Context::Scope scope(rtn->m_context);

								rtn->m_context->InitialiseNulls(program);

								if (auto input = Detail::CrossContextCopy(object_args.GetFirst(), rtn->m_context, true, 0))
								{
									rtn->m_input = input;

									rtn->m_thread = System::Thread::Create(BindMethod(rtn, &ThreadImpl::RunBG, &fn));
								}
							}

							break;
						}
					}
				}
			}
		}

		VM_RTN(rtn);
	});

	if (context & (kContextFlagMain))
	{
		AddFunction(bindings, kGlobal, "CreatePeriodicClock", object_t, { float32_t, bindings->callback_t}, [](Context & context)
		{
			VM_POP(Float32,Detail::FnObject&);

			auto list = PeriodicClock::AcquireList(context);

			VM_RTN(REFLEX_CREATE(PeriodicClockItem, list, args.a, args.a, args.b));
		});
	}
});

