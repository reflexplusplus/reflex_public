#include "reflex_ext/bootstrap/vm_app.h"




REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap::Detail)

struct VmViewWrapper : public View
{
	REFLEX_OBJECT(VmViewWrapper, View);

	static TRef <VmViewWrapper> Create(App & app, const WString::View & path, const ArrayView < Tuple <CString::View, TRef<Reflex::Object>> > & externals, UInt8 flags = VM::kContextFlagUi, const ArrayView <ConstTRef<VM::Module>> & modules = {});

	virtual Pair < TRef <VM::Context>, TRef <GLX::Object> > GetContent() = 0;

	virtual void Rebuild() = 0;



protected:

	VmViewWrapper(App & app);

};

struct VmViewWrapperImpl : public VmViewWrapper
{
	VmViewWrapperImpl(App & app, const WString::View & path, const ArrayView < Tuple <CString::View, TRef<Reflex::Object>> > & externals, UInt8 flags, const ArrayView <ConstTRef<VM::Module>> & modules)
		: VmViewWrapper(app),
		m_monitor(CreateScriptObject(path, modules, externals, [this](VM::Context & context, GLX::Object & object)
		{
			auto chunk = Data::ToBinary(Cast<Data::iStreamable>(*this));

			Clear();

			m_context = context;

			m_object = object;

			m_object->Update();

			m_object->Refresh();

			auto stream = ToView(chunk);

			Data::iStreamable::Deserialize(stream);

			GLX::AddStretch(*this, m_object);

			Data::SetBool(*this, GLX::kresize, Data::GetBool(m_object, GLX::kresize));

			//hack for mobile view -> better solution -> get rid of this wrapper layer, make it a delegate ?

			if (auto style = m_object->GetStyle())
			{
				for (auto & id : GLX::kBgColour)
				{
					auto colour = Make<GLX::ColourProperty>();

					if (GLX::Detail::ExtractProperty(style, id, colour))
					{
						auto style = New<GLX::Style>(kNullKey);

						style->SetProperty(id, colour);

						SetStyle(style);
					}
				}
			}
			else
			{
				SetBounds(m_object, kZeroKey, { 512.0f, 512.0f });
			}

			GLX::AttachAnimationClock(*this, GLX::WindowClient::kRequestAutoFit, [this](Float32)
			{
				GLX::Emit(*this, GLX::WindowClient::kRequestAutoFit);

				GLX::DetachClock(*this, GLX::WindowClient::kRequestAutoFit);
			});
		}))
	{
	}

	Pair < TRef <VM::Context>, TRef <GLX::Object> > GetContent() override
	{
		return { m_context, m_object };
	}

	virtual void OnResetState(Key32 context) override
	{
		auto program = m_context->program;

		if (auto onreset = VM::QueryFunction(program, { VM::kGlobal, K32("OnReset") }, program->bindings->void_t, {}))
		{
			VM::CallReturningVoid(m_context, *onreset);
		}
	}
	
	virtual void OnRestoreState(Data::Archive::View & stream, Key32 context) override
	{
		if (stream)
		{
			auto program = m_context->program;

			auto bindings = program->bindings;

			if (auto archiveobject_t = VM::GetType<Data::ArchiveObject>(bindings))
			{
				auto onrestore = VM::QueryFunction(program, { VM::kGlobal, K32("OnRestore") }, bindings->void_t, { archiveobject_t });

				if (onrestore)
				{
					auto binary = AutoRelease(New<Data::ArchiveObject>(stream));

					VM::CallReturningVoid(m_context, *onrestore, binary.Adr());

					return;
				}
			}
		}

		OnResetState(context);
	}

	virtual void OnStoreState(Data::Archive & stream) const override
	{
		auto program = m_context->program;

		auto & bindings = program->bindings;

		if (auto archiveobject_t = VM::GetType<Data::ArchiveObject>(bindings))
		{
			auto onstore = VM::QueryFunction(program, { VM::kGlobal, K32("OnStore") }, archiveobject_t, {});

			if (onstore)
			{
				auto chunkref = AutoRelease(VM::CallReturningObject<Data::ArchiveObject>(m_context, *onstore));

				auto & chunk = chunkref->value;

				auto size = chunk.GetSize();

				MemCopy(chunk.GetData(), Extend(stream, size).data, size);
			}
		}
	}
	
	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kKeyDown)
		{
			switch (GLX_KEY_CODE(GLX::GetKeyCode(e), GLX::GetModifierKeys(e)))
			{
			case GLX_KEY_CODE(GLX::kKeyCodeD, GLX::kModifierKeyShift | GLX::kModifierKeyPrimary):
				global->EnableIde(!global->IdeEnabled());
				return true;

			case GLX_KEY_CODE(GLX::kKeyCodeF5, GLX::kModifierKeyNone):
				Rebuild();
				return true;

			default:
				break;
			}
		}

		return GLX::Object::OnEvent(src, e);
	}

	virtual void Rebuild() override
	{
		File::ResourcePool::Lock lock(global->resourcepool);

		m_monitor->ForceRebuild(lock);
	}

	Reference <VM::Context> m_context;

	Reference <GLX::Object> m_object;

	Reference <IDE::ResourceGroup> m_monitor;
};

REFLEX_END_INTERNAL

Reflex::Bootstrap::Detail::VmViewWrapper::VmViewWrapper(App & app)
	: View(app, 1, {})
{
}

Reflex::TRef <Reflex::Bootstrap::Detail::VmViewWrapper> Reflex::Bootstrap::Detail::VmViewWrapper::Create(App & app, const WString::View & path, const ArrayView < Tuple <CString::View, TRef<Reflex::Object>> > & externals, UInt8 context_flags, const ArrayView <ConstTRef<VM::Module>> & modules)
{
	return REFLEX_CREATE(VmViewWrapperImpl, app, path, externals, context_flags, modules);
}
