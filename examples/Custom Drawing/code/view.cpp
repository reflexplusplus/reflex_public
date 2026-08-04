#include "view.h"
#include "widget.h"




//
//CustomDrawing::View implementation

using namespace Reflex;

REFLEX_BEGIN_INTERNAL(CustomDrawing)

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

	Reference <GLX::Object> m_widget;

};

ViewImpl::ViewImpl(App & app)
	: View(app, kChunkVersion, L":res:CustomDrawing/styles.glx")
	, app(app)
	, m_widget(CreateWidget())
{
	Data::SetBool(*this, GLX::kresize, true);

	GLX::AddFloat(*this, m_widget, GLX::kAlignmentCenter);
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
	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	m_widget->SetStyle(style["Display"]);
}

void ViewImpl::OnUpdate()
{
	//this is called automatically when app state changes
}

REFLEX_END_INTERNAL

TRef <CustomDrawing::View> CustomDrawing::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
