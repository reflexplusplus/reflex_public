#include "reflex_ext/bootstrap/vm_app.h"




//const VM::Module Reflex::Bootstrap::gView(K32("Control"), VM::kContextFlagUi, [](VM::Compiler::State & cstate, UInt8 contextflags, Object & object)
//{
//	cstate.Instantiate(GLXVM::gGLX);
//
//	cstate.Instantiate(AudioPlugin::gBindings);
//
//	auto & bindings = cstate.bindings;
//
//	auto void_t = bindings.void_t;
//	auto int32_t = bindings.int32_t;
//	auto audioplugin_t = VM::GetType<Bootstrap::AudioPlugin>(bindings);
//
//	if (audioplugin_t)	//for documentation
//	{
//		VM::AddFunction(bindings, kNullKey, "CreateControl", VM::GetType<GLX::Object>(bindings), { audioplugin_t, int32_t }, [](VM::Context & context)
//		{
//			VM_POP(AudioPlugin&, UInt32);
//
//			auto values = args.a.GetParameterValues();
//
//			if (args.b < values.b)
//			{
//				VM_RTN(Control::Create(args.a, args.b));
//			}
//			else
//			{
//				Bootstrap::Value32 unused;
//
//				VM_RTN(Control::Create(Null<Bootstrap::ParamInfo>(), unused));
//			}
//		});
//	}
//});
