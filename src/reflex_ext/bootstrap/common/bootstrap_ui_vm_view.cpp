#include "[require].h"
#include "../../../../include/reflex_ext/vm.h"




Reflex::Unretained <Reflex::IDE::ResourceGroup> Reflex::Bootstrap::CreateScriptObject(const WString::View & path, UInt8 context_flags, const ArrayView <ConstRef<VM::Module>> & modules, const ArrayView < Tuple <CString::View, AlreadyRetained<Object>> > & externals, const Function <void(VM::Context & context, GLX::Object & object)> & on_create)
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

		UInt8 context_flags;

		Array < ConstRef <VM::Module> > modules;

		Array < Tuple < CString::View, AlreadyRetained<Object> > > externals;

		Function <void(VM::Context & context, GLX::Object & object)> on_create;

		Reference <VM::Context> context;

		Reference <GLX::Object> self;
	};

	auto state = Reflex::Make<State>();

	state->path = path;

	state->context_flags = context_flags;

	state->modules.Append(modules);

	state->modules.Push(GLXVM::g_glx);

	state->on_create = on_create;

	state->externals = externals;

	auto onreload = [resourcepool, compiler, uid, state](IDE::ResourceGroup & monitor)
	{
		GLX::Core::Context context;

		File::ResourcePool::Lock lock(resourcepool);

		auto prebindings = Make<VM::Compiler::Context>(New<VM::Bindings>(state->context_flags));

		for (auto & module : state->modules) prebindings->Instantiate(module);

		if (!VM::RegisterGlobals(prebindings->bindings, state->externals)) return;

		auto self_t = VM::QueryType<GLX::Object>(prebindings->bindings);

		if (!self_t) return;

		prebindings->bindings->RegisterGlobal(self_t, VM::kGlobal, "self", VM::Bindings::Global::kFlagsConst);

		auto vm_program = AutoRelease(compiler->Compile(lock, state->path, state->context_flags, prebindings));

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

		if (!vm_program->Status()) return;

		auto vm_context = VM::Context::Create(*vm_program, { .context_id = GLX::Core::desktop->GetContextID() });

		Array < Tuple < CString::View, AlreadyRetained<Object> > > externals;

		externals = state->externals;

		auto self = REFLEX_CREATE(GLXVM::Object, vm_context);

		externals.Push({ "self", self });

		if (!VM::SetGlobals(*vm_context, externals)) return;

		if (!vm_context->Run()) return;

		state->self->Detach();

		state->context = vm_context;

		state->self = self;

		state->on_create(vm_context, *self);
	};

	auto monitor = IDE::ResourceGroup::Create(resourcepool, uid, path, onreload);

	onreload(*monitor);

	return monitor;
}
