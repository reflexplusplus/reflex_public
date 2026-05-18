#include "view.h"




//
//_PRODUCT-NAME-SYMBOL_::View implementation

namespace _PRODUCT-NAME-SYMBOL_ { namespace {	//begin internal namespace

using namespace Reflex;

struct ViewImpl : public View
{
public:

	static constexpr UInt16 kChunkVersion = 0;

	ViewImpl(App & app);


	//Bootstrap::View callbacks

	void OnResetState(Key32 context) override;

	void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	void OnStoreState(Data::Archive & stream) const override;

	
	//GLX::Object callbacks

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnSetStyle(const GLX::Style & style) override;

	void OnUpdate() override;



	const TRef <App> app;


	//put your GLX::Object members here


	GLX::Button m_ide;
};

ViewImpl::ViewImpl(App & app)
	: View(app, kChunkVersion, L":res:_PRODUCT-NAME-SYMBOL_/styles.glx")
	, app(app)
{
	Data::SetBool(*this, GLX::kresize, true);

	if constexpr (REFLEX_DEBUG)
	{
		GLX::SetText(m_ide, L"Console");
		
		GLX::AddFloat(*this, m_ide, GLX::kAlignmentTopRight);
	}
}

void ViewImpl::OnResetState(Key32 context)
{
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
	//if you change/add data here, you will need to increment kChunkVersion, as previous chunks will be incompatible
}

bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		if constexpr (REFLEX_DEBUG)
		{
			if (src == m_ide) Bootstrap::global->EnableIde(!Bootstrap::global->IdeEnabled());
		}

		return true;
	}

	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto button = style["Button"];

	if constexpr (REFLEX_DEBUG) m_ide.SetStyle(button);
}

void ViewImpl::OnUpdate()
{
	//this is called automatically when app state changes

	if constexpr (REFLEX_DEBUG) GLX::Select(m_ide, Bootstrap::global->IdeEnabled());
}

} }	//end internal namespace

Reflex::TRef <_PRODUCT-NAME-SYMBOL_::View> _PRODUCT-NAME-SYMBOL_::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
