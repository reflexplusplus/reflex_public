#include "reflex_ext.h"

using namespace Reflex;



REFLEX_BEGIN_INTERNAL(ReflexProjectCreator)

const VM::Module g_glx_ext("GLX > Ext", { GLXVM::gGLX }, VM::kContextFlagUi, [](VM::Compiler::State & state, UInt8 contextflags, Object & object)
{
	constexpr Key32 kGLX = "GLX";

	auto bindings = state.bindings;

	auto void_t = bindings->void_t;
	auto object_t = VM::GetType<GLX::Object>(bindings);

	//auto scroller_t = VM::GetType<GLX::Scroller>(bindings);

	VM::AddFunction(bindings, kGLX, "CreateWidget", object_t, { bindings->key32_t }, [](VM::Context & context)
	{
		VM_POP1(UInt32);

		Object * rtn;

		switch (arg)
		{
		case K32("TextEdit"):
			rtn = New<GLXVM::WidgetOf<GLX::TextArea>>(context, false).Adr();
			break;

		case K32("Button"):
			rtn = New<GLXVM::WidgetOf<GLX::Button>>(context).Adr();
			break;

		case K32("Popup"):
			rtn = New<GLXVM::WidgetOf<GLX::Popup>>(context).Adr();
			break;

		default:
			rtn = context.GetNullByRTTID(REFLEX_TYPEID(GLX::Object), nullptr);
			break;
		}

		VM_RTN(rtn);
	});

	VM::AddFunction(bindings, kGLX, "AddMenuItem", object_t, { object_t, bindings->string_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&, VM::String&);

		Object * rtn;

		if (auto menu = DynamicCast<GLX::Menu>(args.a))
		{
			auto item = menu->AddItem(args.b.GetView());

			GLXVM::AttachCircular(item, context);

			rtn = item.Adr();
		}
		else
		{
			rtn = context.GetNullByRTTID(REFLEX_TYPEID(GLX::Object), nullptr);
		}

		VM_RTN(rtn);
	});

	VM::AddFunction(bindings, kGLX, "AddMenuSeparator", void_t, {}, [](VM::Context & context)
	{
		VM_POP1(GLX::Object&);

		if (auto menu = DynamicCast<GLX::Menu>(arg))
		{
			menu->AddSeparator();
		}
	});
});

const VM::Module g_global_functions("ReflexProjectCreator", {}, kMaxUInt8, [](VM::Compiler::State & state, UInt8 contextflags, Object & object)
{
	constexpr Key32 kNS = "ReflexProjectCreator";

	VM::BindConstant(state, kNS, "kDebug", UInt8(REFLEX_DEBUG));

	VM::AddFunction(state.bindings, kNS, "GetPlatformName", state.bindings->string_t, {}, [](VM::Context & context)
	{
		switch (System::kPlatform)
		{
		case System::kPlatformWindows:
			VM_RTN(VM::String::Create(ToView(L"win")));
			break;

		case System::kPlatformMacOS:
			VM_RTN(VM::String::Create(ToView(L"macos")));
			break;

		case System::kPlatformIOS:
			VM_RTN(VM::String::Create(ToView(L"ios")));
			break;

		case System::kPlatformAndroid:
			VM_RTN(VM::String::Create(ToView(L"android")));
			break;

		default:
			VM_RTN(K32("other"));
			break;
		};
	});

	VM::AddFunction(state.bindings, kNS, "FindReflexPath", state.bindings->string_t, {}, [](VM::Context & context)
	{
		auto current_directory = System::GetCurrentDirectory();

		auto segments = Split(current_directory, System::kPathDelimiter);

		if (auto idx = Search<CaseInsensitive>(segments, L"reflex"))
		{
			segments.SetSize(idx.value + 1);

			auto reflex_path = Merge(segments, System::kPathDelimiter);

			reflex_path.Push(System::kPathDelimiter);

			VM_RTN(VM::String::Create(reflex_path));
		}
		else
		{
			VM_RTN(VM::String::null);
		}
	});
});

Reference <VM::Context> CreateAndRun(File::ResourcePool::Lock & lock, VM::Compiler & compiler, const WString::View & path)
{
	//kContextFlagMainBg need this so Sleep can work

	auto program = compiler.Compile(lock, path, VM::kContextFlagMainBg, Object::null, { VM::gDataPropertySet });

	auto bindings = program->bindings;

	auto context = New<VM::Context>();

	context->Initialise(program);

	return context;
}

REFLEX_END_INTERNAL

TRef <Object> System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	constexpr auto GetArgs = [](const ArrayView <CString::View> & cmdline)
	{
		Data::PropertySet args;

		UInt count = cmdline.size & ~1u;

		UInt idx = 0;

		while (idx < count)
		{
			auto key = Mid<true>(cmdline[idx++], 2);

			auto value = cmdline[idx++];

			Data::SetCString(args, key, value);
		}

		return args;
	};

	auto GetFolderArg = [](const Data::PropertySet & args, Key32 id)
	{
		return File::CorrectTrailingStroke(File::CorrectStrokes(ToWString(Data::GetCString(args, id))));
	};


	constexpr CString::View kVendor = "Reflex Multimedia";
	constexpr CString::View kProduct = "Reflex Project Creator";
	constexpr Key32 kResourcesSubDomain = K32("ReflexProjectCreator");

	if (cmdline)
	{
		auto global = AutoRelease(Bootstrap::Global::Acquire(kVendor, kProduct, Bootstrap::Detail::ExtractProjectDir(__FILE__), kResourcesSubDomain));

		auto vm = AutoRelease(VM::Start());

		auto compiler = Make<VM::Compiler>();

		File::ResourcePool::Lock lock(global->resourcepool);

		
		auto install_context = ReflexProjectCreator::CreateAndRun(lock, compiler, L":res:ReflexProjectCreator/install.h");

		auto install_program = install_context->program;

		auto install_bindings = install_program->bindings;

		auto install_propertyset_t = install_bindings->GetTypeByRTTID(GetTypeID<Data::PropertySet>());

		auto template_t = install_bindings->GetTypeByRTTID(VM::GenerateScriptTypeID("Template", L":res:ReflexProjectCreator/data.h"));

		auto open_template = VM::QueryFunction(install_program, { VM::kGlobal, K32("OpenTemplate") }, template_t, { install_bindings->string_t });

		auto encode_engine_args = VM::QueryFunction(install_program, { VM::kGlobal, K32("EncodeEngineArgs") }, install_propertyset_t, { template_t, install_propertyset_t, install_bindings->string_t });


		auto args = GetArgs(cmdline);

		auto template_path = Join(GetFolderArg(args, "template"), L"install.cfg");

		auto tmpl = AutoRelease(VM::CallReturningObject<Object>(install_context, *open_template, *Make<VM::String>(template_path)));

		auto dest = Make<VM::String>(GetFolderArg(args, "dest"));

		Data::PropertySet variables;

		Data::SetCString(variables, "VENDOR-NAME", Data::GetCString(args, "vendor_name", "vendor"));
		Data::SetCString(variables, "PRODUCT-NAME", Data::GetCString(args, "product_name", "product"));

		auto engine_args = AutoRelease(VM::CallReturningObject<Data::PropertySet>(install_context, *encode_engine_args, *tmpl, variables, *dest));


		auto engine_context = ReflexProjectCreator::CreateAndRun(lock, compiler, L":res:ReflexProjectCreator/engine.c");

		auto engine_program = engine_context->program;

		auto engine_bindings = engine_program->bindings;

		auto engine_propertyset_t = engine_bindings->GetTypeByRTTID(GetTypeID<Data::PropertySet>());

		auto tuple_string_t = engine_bindings->GetTypeByRTTID(K32("Tuple@(String,String)"));

		auto instantiate_template = VM::QueryFunction(engine_program, { VM::kGlobal, K32("InstallTemplateWrapper") }, tuple_string_t, { engine_propertyset_t });

		auto pair = AutoRelease(VM::CallReturningObject<Object>(engine_context, *instantiate_template, *engine_args));


		return Object::null;
	}
	else
	{
		return Bootstrap::StartVmApp
		(
			config,
			kVendor,
			kProduct,
			kResourcesSubDomain,
			__FILE__,
			L":res:ReflexProjectCreator/main.c",
			L":res:ReflexProjectCreator/view.c"
		);
	}
}
