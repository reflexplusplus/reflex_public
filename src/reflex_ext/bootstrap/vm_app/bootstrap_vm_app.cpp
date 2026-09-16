#include "reflex_ext/bootstrap/vm_app.h"
#include "vm_view_wrapper.h"
#include "../common/[require].h"




//
//App

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

struct VmAppDelegate : public App
{
	static const VM::Module g_bindings;

	REFLEX_OBJECT(VmAppDelegate, App);

	static Unretained <VmAppDelegate> Create(UInt32 magicbytes, const WString::View & path)
	{
		return New<VmAppDelegate>(magicbytes, path);
	}

	VmAppDelegate(UInt32 magicbytes, const WString::View & path)
		: App(magicbytes, 1),
		m_vm(VM::Start()),
		m_compiler(VM::Compiler::Create()),
		m_path(path),
		m_onreset(nullptr),
		m_onrestore(nullptr),
		m_buildcount(0)
	{
		Compile();
	}

	void Compile(AlreadyRetained <IDE::ResourceGroup> resources = {})
	{
		File::ResourcePool::Lock lock(global->resourcepool);

		Array < Tuple < CString::View, AlreadyRetained<Object> > > externals;

		externals.Push({ "prefs", global->prefs });

		externals.Push({ "self", this });

		auto prebindings = Make<VM::Compiler::Context>(New<VM::Bindings>(VM::kContextFlagMain));

		prebindings->Instantiate(g_bindings);

		if (!Detail::RegisterScriptGlobals(prebindings->bindings, externals)) return;

		auto program = AutoRelease(m_compiler->Compile(lock, m_path, VM::kContextFlagMain, prebindings));

		if (IDE::kIsAwake)
		{
			if (!resources)
			{
				auto new_resources = New<IDE::ResourceGroup>(lock.resourcepool, K32("instance"), L"instance", [this](IDE::ResourceGroup & self)
				{
					auto data = Data::ToBinary(*this);

					Compile(self);

					Data::FromBinary(data, *this);
				});

				m_resourcemonitor = new_resources;
				resources = NoRetain(new_resources);
			}

			for (auto & i : program->sources) resources->AddItem(i.address, i.object);
		}

		if (!program->Status()) return;

		Unload();

		auto contextid = Reflex::Detail::AcquireContextID();

		RemoveConst(Object::GetContextID()) = contextid;

		RemoveConst(global->prefs->Object::GetContextID()) = contextid;

		Reflex::Detail::Constructor<Data::PropertySet>::Reconstruct(m_shared, contextid);

		m_onreset = nullptr;
		
		m_binaryobject_t = nullptr;
		
		m_onrestore = nullptr;

		{
			m_context = VM::Context::Create(*program, { .context_id = contextid });

			m_context->SetProperty(kNullKey, lock.resourcepool);	//For VM::Thread

			VM::Context::Scope scope(*m_context);

			if (!Detail::SetScriptGlobals(*m_context, externals)) return;

			if (!m_context->Run()) return;

			auto bindings = program->bindings;

			auto void_t = bindings->void_t;

			m_onreset = VM::QueryFunction(program, { VM::kGlobal, K32("OnReset") }, void_t, {});

			if ((m_binaryobject_t = VM::QueryType<Data::ArchiveObject>(bindings)))
			{
				m_onrestore = VM::QueryFunction(program, { VM::kGlobal, K32("OnRestore") }, void_t, { m_binaryobject_t });
			}
		}

		RemoveConst(m_buildcount)++;
	}

	void Unload()
	{
		auto program = m_context->program;

		if (auto onunload = VM::QueryFunction(program, { VM::kGlobal, K32("OnUnload") }, program->bindings->void_t, {}))
		{
			VM::Call<void>(m_context, *onunload);
		}
	}

	void OnReset(Key32 context) override
	{
		if (m_onreset)
		{
			VM::Call<void>(m_context, *m_onreset);
		}
	}

	void OnRestore(Data::Archive::View & chunk, Key32 context) override
	{
		if (chunk && m_onrestore)
		{
			auto binary = AutoRelease(New<Data::ArchiveObject>(chunk));

			VM::Call<void>(m_context, *m_onrestore, binary.Adr());
		}
		else
		{
			OnReset(context);
		}
	}
		
	void OnStore(Data::Archive & stream) const override
	{
		auto program = m_context->program;
		
		if (auto onstore = VM::QueryFunction(program, { VM::kGlobal, K32("OnStore") }, m_binaryobject_t, {}))
		{
			auto chunkref = AutoRelease(VM::Call<Data::ArchiveObject>(m_context, *onstore));
			
			Data::Archive::View chunk = chunkref->value;

			MemCopy(chunk.data, Extend(stream, chunk.size).data, chunk.size);
		}
	}
	
	void OnUnsetProperty(Address adr) override
	{
		m_shared.UnsetProperty(adr);
	}

	void OnSetProperty(Address adr, Object & object) override
	{
		m_shared.SetProperty(adr, object);
	}

	void OnQueryProperty(Address adr, Object * & pobject) const override
	{
		pobject = m_shared.QueryProperty(adr, pobject);
	}

	void OnReleaseData() override
	{
		App::OnReleaseData();

		Unload();

		Reflex::Detail::Constructor<Data::PropertySet>::Reconstruct(m_shared, 0);

		m_context.Clear();
	}

	static void BindVM(VM::Compiler::Context & cstate, UInt8 contextflags);

	using App::Notify;

	
	Reference <Object> m_vm;

	Reference <VM::Compiler> m_compiler;

	WString m_path;

	Reference <VM::Context> m_context;

	const VM::ScriptFunction * m_onreset, * m_onrestore;

	VM::TypeRef m_binaryobject_t;
	
	Data::PropertySet m_shared;

	Reference <Object> m_resourcemonitor;

	UInt32 m_buildcount;	//this is used by view to rebuild after main rebuilds

};

void Reflex::Bootstrap::VmAppDelegate::BindVM(VM::Compiler::Context & cstate, UInt8 contextflags)
{
	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;
	auto bool_t = bindings->bool_t;
	auto string_t = bindings->string_t;



	//publish instance

	auto app_t = VM::RegisterObject<VmAppDelegate>(bindings, kNullKey, "Instance");

	VM::Detail::SetTypeFlag(app_t, VM::Type::kFlagDefaultConstructable, false);	//prevent script creating instances

	VM::Detail::SetTypeFlag(app_t, VM::Type::kFlagExplicitNullable, false);	//prevent script creating instances


	VM::AddMethod(bindings, "GetFilename", string_t, { app_t }, [](VM::Context & context)
	{
		auto app = VM::Pop<VmAppDelegate>(context);

		VM::Return(context, New<VM::String>(app->GetFilename()));
	});


	VM::Detail::BindObjectMethod<&App::IsEdited>(bindings, app_t, "IsEdited");


	VM::AddMethod(bindings, "Reset", void_t, { app_t }, [](VM::Context & context)
	{
		auto app = VM::Pop<VmAppDelegate>(context);

		app->session->Reset();
	});

	VM::AddMethod(bindings, "Open", bool_t, { app_t, string_t }, [](VM::Context & context)
	{
		auto [app, filename] = VM::Pop<VmAppDelegate,VM::String>(context);
			
		VM::Return(context, app->Open(filename->GetView()));
	});

	VM::AddMethod(bindings, "Save", bool_t, { app_t, string_t }, [](VM::Context & context)
	{
		auto [app, filename] = VM::Pop<VmAppDelegate, VM::String>(context);

		VM::Return(context, app->Save(filename->GetView()));
	});

		
	VM::Detail::BindPropertySetInterface(cstate, app_t);

	app_t->null = [](VM::Context & context, VM::TypeRef type) -> Object &
	{
		//Instance has no type-correct null object. Context construction needs a
		//temporary value for the registered global slot before the host can bind it.
		//SetGlobal replaces this placeholder before Run, and Instance is non-nullable,
		//so script code cannot explicitly obtain it.
		return Object::null;
	};


	if (contextflags & VM::kContextFlagMain)
	{
		VM::AddMethod(bindings, "Notify", void_t, { app_t, bool_t }, [](VM::Context & context)
		{
			auto [app, edited] = VM::Pop<VmAppDelegate,bool>(context);
				
			app->Notify(edited);
		});
	}
}

REFLEX_END_INTERNAL

Reflex::Unretained <Reflex::Bootstrap::Global> Reflex::Bootstrap::StartVmApp(System::App::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry, const WString::View & main, const WString::View & view)
{
	auto global = StartApp<VmAppDelegate>(config, vendor, product, resources_subdomain, entry, resources_subdomain.value, main);
	
	Detail::PublishAppView(config, [path = WString(view)](Object & object) -> Unretained <GLX::Object>
	{
		auto app = Cast<VmAppDelegate>(object);

		auto viewwrapper = VmViewWrapper::Create(app, path, { { "app", NoRetain(app) } }, VM::kContextFlagUi, { Bootstrap::VmAppDelegate::g_bindings });

		viewwrapper->GetContent().b->Update();

		viewwrapper->SetProperty(K32("monitor"), GLX::CreateAnimationClock([viewwrapper = NoRetain(viewwrapper), app = NoRetain(app), count_z = app->GetCurrentCount(), buildcount_z = app->m_buildcount](Float32 delta) mutable
		{
			if (SetFiltered(buildcount_z, app->m_buildcount))
			{
				viewwrapper->Rebuild();
			}
			else if (SetFiltered(count_z, app->GetCurrentCount()))
			{
				viewwrapper->GetContent().b->Update();
			}
		}));

		return std::move(viewwrapper);
	});
	
	return global;
}

const Reflex::VM::Module Reflex::Bootstrap::VmAppDelegate::g_bindings("Bootstrap::App", { VM::g_data_propertyset }, VM::kContextFlagMain | VM::kContextFlagUi, &VmAppDelegate::BindVM);

//void Reflex::Bootstrap::AudioPlugin::BindVM(VM::Compiler::Context & cstate, UInt8 contextflags, Object & client, const CString::View & self)
//{
//	if (auto audioplugin = DynamicCast<AudioPlugin>(client))
//	{
//		auto & bindings = cstate.bindings;
//
//		auto void_t = bindings.void_t;
//
//		auto int32_t = bindings.int32_t;
//
//
//		//publish instance
//
//		auto audioplugin_t = VM::RegisterObject<AudioPlugin>(bindings, kNullKey, "Instance");
//
//		VM::Detail::SetTypeFlag(audioplugin_t, VM::Detail::Type::kFlagDefaultConstructable, false);	//prevent script creating instances
//
//		VM::Detail::SetTypeFlag(audioplugin_t, VM::Detail::Type::kFlagExplicitNullable, false);	//prevent script creating instances
//
//
//		VM::Detail::BindDynamicPropertyInterface(cstate, audioplugin_t);
//
//		audioplugin_t->null = [](VM::Context & context, VM::TypeRef type) -> Object &
//		{
//			return context.clientdata;
//		};
//
//
//		auto & prefs = audioplugin_t->members.Push();
//
//		prefs.a = K32("prefs");
//		prefs.b = { VM::GetType<Data::PropertySet>(bindings), VM::Detail::Variable::Location::kLocationMember, VM_OFFSETOF(AudioPlugin,m_prefs), true };
//
//		auto & session = audioplugin_t->members.Push();
//
//		session.a = K32("session");
//		session.b = { VM::GetType<Data::PropertySet>(bindings), VM::Detail::Variable::Location::kLocationMember, VM_OFFSETOF(AudioPlugin,m_session), true };
//
//		if (contextflags & VM::kContextFlagMain)
//		{
//			VM::Detail::BindObjectMethod<&AudioPlugin::Notify>(bindings, audioplugin_t, "Notify");
//		}
//
//		VM::AddMethod(bindings, "GetNumParameter", int32_t, { audioplugin_t }, [](VM::Context & context)
//		{
//			VM_POP1(AudioPlugin&);
//
//			VM_RTN(arg.GetParameterValues().b);
//		});
//
//
//		VM::RegisterExternalObject<AudioPlugin>(cstate, audioplugin_t, kNullKey, self, audioplugin);
//
//
//
//		auto audiodelegate_monitor_t = VM::RegisterObject<AudioPlugin::MonitorObject>(bindings, K32("Instance"), "Monitor");
//
//		VM::AddConstructor(bindings, audiodelegate_monitor_t, { audioplugin_t }, [](VM::Context & context)
//		{
//			VM_POP1(AudioPlugin&);
//
//			VM_RTN(REFLEX_CREATE(AudioPlugin::MonitorObject, arg));
//		});
//
//		VM::Detail::BindObjectMethod<&AudioPlugin::MonitorObject::Poll>(bindings, audiodelegate_monitor_t, "Poll");
//	}
//}
