#pragma once

#include "reflex_ext/bootstrap/vm_app.h"





namespace Reflex::Bootstrap
{

	struct VmViewWrapper : public View
	{
		REFLEX_OBJECT(VmViewWrapper, View);

		static TRef <VmViewWrapper> Create(App & app, const WString::View & path, const ArrayView < Tuple <CString::View, TRef<Reflex::Object>> > & externals, UInt8 flags = VM::kContextFlagUi, const ArrayView <ConstTRef<VM::Module>> & modules = {});

		virtual Pair < TRef <VM::Context>, TRef <GLX::Object> > GetContent() = 0;

		virtual void Rebuild() = 0;



	protected:

		VmViewWrapper(App & app);

	};

}

