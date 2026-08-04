#include "vm_view.h"





//
//impl

const Reflex::VM::Module Reflex::Bootstrap::g_externals("Bootstrap::Externals", {}, kMaxUInt8, [](VM::Compiler::State & state, UInt8, Object & clientdata)
{
	TRef externals_object = Cast<ScriptExternals>(clientdata);

	for (auto & i : externals_object->objects)
	{
		auto super = i.b->object_t;

		do
		{
			if (auto object_t = state.bindings->GetTypeByRTTID(super->type_id))
			{
				VM::RegisterExternalObject<false>(state, object_t, VM::kGlobal, i.a, i.b);

				break;
			}

			super = super->base;
		}
		while (super && super != Object::kDynamicTypeInfo);
	}
});

Reflex::TRef <Reflex::IDE::ResourceGroup> Reflex::Bootstrap::CreateScriptObject(const WString::View & path, const ArrayView <ConstTRef<VM::Module>> & modules, const ArrayView < Tuple <CString::View, TRef<Object>> > & externals, const Function <void(VM::Context & context, GLX::Object & object)> & on_create)
{
	auto resourcepool = global->resourcepool;

	auto compiler = global->QueryProperty<VM::Compiler>(K32("Compiler"));

	if (!compiler)
	{
		VM::Start();

		compiler = VM::Compiler::Create().Adr();

		global->SetProperty(K32("Compiler"), compiler);
	}

	Key32 uid = path;

	struct State : public Object
	{
		WString path;

		Array < ConstTRef <VM::Module> > modules;

		Array < Tuple < CString::View, TRef<Object> > > externals;

		Function <void(VM::Context & context, GLX::Object & object)> on_create;

		Reference <VM::Context> context;

		Reference <GLX::Object> self;
	};

	auto state = Reflex::Make<State>();

	state->path = path;

	state->modules.Append(modules);

	state->modules.Append({ GLXVM::gGLX, g_externals });

	state->on_create = on_create;

	state->externals = externals;

	auto onreload = [resourcepool, compiler, uid, state](IDE::ResourceGroup & monitor)
	{
		GLX::Core::Context context;

		state->self->Detach();

		auto vm_context = VM::Context::Create(GLX::Core::desktop->GetContextID());

		state->context = vm_context;

		TRef self = REFLEX_CREATE(GLXVM::Object, vm_context);

		state->self = self;

		File::ResourcePool::Lock lock(resourcepool);

		ScriptExternals externals;

		externals.objects = state->externals;

		externals.objects.Push({ "self", self });

		auto vm_program = compiler->Compile(lock, state->path, VM::kContextFlagUi, externals, state->modules);

		vm_context->Initialise(vm_program);

		monitor.Clear();

		for (auto & i : vm_program->sources)
		{
			if (auto stylesheet = DynamicCast<GLX::StyleSheet>(i.object))
			{
				IDE::AddStyleSheet(monitor, *stylesheet);
			}
			else
			{
				monitor.AddItem(i.address, i.object);
			}
		}

		state->on_create(vm_context, self);
	};

	auto monitor = IDE::ResourceGroup::Create(resourcepool, uid, path, onreload);

	onreload(*monitor);

	return monitor;
}
