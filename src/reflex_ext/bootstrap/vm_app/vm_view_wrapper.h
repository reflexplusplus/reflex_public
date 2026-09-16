#pragma once

#include "reflex_ext/bootstrap/vm_app.h"





namespace Reflex::Bootstrap
{

	struct VmViewWrapper : public View
	{
		REFLEX_OBJECT(VmViewWrapper, View);

		static Unretained <VmViewWrapper> Create(App & app, const WString::View & path, const ArrayView < Tuple <CString::View, AlreadyRetained<Reflex::Object>> > & externals, UInt8 flags = VM::kContextFlagUi, const ArrayView <ConstRef<VM::Module>> & modules = {});

		virtual Pair < AlreadyRetained <VM::Context>, AlreadyRetained <GLX::Object> > GetContent() = 0;

		virtual void Rebuild() = 0;



	protected:

		VmViewWrapper(App & app);

	};

}

