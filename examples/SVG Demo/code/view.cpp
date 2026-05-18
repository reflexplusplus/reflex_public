#include "view.h"




//
//SVGDemo::View implementation

using namespace Reflex;

REFLEX_BEGIN_INTERNAL(SVGDemo)

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
	: View(app, kChunkVersion, L":res:SVGDemo/styles.glx")
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
			if (src == m_ide)
			{
				Bootstrap::global->EnableIde(!Bootstrap::global->IdeEnabled());

				return true;
			}
		}

		if (GLX::GetClickFlags(e) & GLX::kClickFlagRmb)
		{
			app->SetPath({});
		}
		else if (auto path = GLX::ShowFileDialog(Bootstrap::global->prefs, "svg_location", false, { L"svg" }))
		{
			app->SetPath(path);
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
	if constexpr (REFLEX_DEBUG) GLX::Select(m_ide, Bootstrap::global->IdeEnabled());

	auto xml_bytes = File::Open(app->GetPath());

	auto xml = Make<Data::PropertySet>(Data::DecodePropertySet(Data::kReflexXmlFormat, xml_bytes));	//use a reference as we need to keep alive

	if (auto svgs = GLX::Detail::InspectSVG(xml))
	{
		auto svg = svgs.GetFirst();									//root or first of multi-icon set

		GLX::SetColorCanvas(*this, {}, [this, xml, svg](GLX::ColorCanvasContext & ctx)	//!capture xml is critical to keep xml alive
		{
			GLX::Detail::DecodeSVG(ctx.output, svg, ctx.size);	//can also just decode once to kNormalSize but decoding to actual size is best for highest quality
		});

		ClearState("empty");
	}
	else
	{
		GLX::UnsetCanvas(*this, {});

		SetState("empty");
	}
}

REFLEX_END_INTERNAL

TRef <SVGDemo::View> SVGDemo::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
