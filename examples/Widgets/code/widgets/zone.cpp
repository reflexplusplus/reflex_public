#include "widgets.h"




//
// Zone demo

namespace Widgets { namespace {

struct Zone : public GLX::Object
{
	Zone() 
		: m_resize(GLX::ResizeBehaviour::Create())
	{
		SetDelegate(GLX::ResizeBehaviour::kClassID, m_resize);
		SetDelegate(GLX::MoveBehaviour::kClassID, GLX::MoveBehaviour::Create());
		m_resize->SetRect({ { 0.0f, 0.0f }, { 120.0f, 64.0f } });
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kTransaction)
		{
			auto rect = m_resize->GetRect();
			output.Log("zone stage=", UInt(GLX::GetTransactionStage(e)), " edge=", UInt(GLX::GetIndex(e)), " rect=", rect.origin.x, ",", rect.origin.y, " ", rect.size.w, "x", rect.size.h);
			if (GLX::GetTransactionStage(e) == GLX::kTransactionStagePerform)
			{
				rect.origin.x = Quantise(Max(rect.origin.x, 0.0f), 8.0f); rect.origin.y = Quantise(Max(rect.origin.y, 0.0f), 8.0f);
				rect.size.w = Max(Quantise(rect.size.w, 8.0f), 24.0f); rect.size.h = Max(Quantise(rect.size.h, 8.0f), 24.0f);
				m_resize->SetRect(rect);
			}
			return true;
		}
		return Object::OnEvent(src, e);
	}

	const TRef <GLX::ResizeBehaviour> m_resize;
};

} }

const Widgets::DemoDesc Widgets::g_zone = Widgets::MakeDemo("zone", L"Zone", true, []() -> Unretained <GLX::Object>
{
	auto container = New<GLX::Object>();

	auto zone = New<Zone>();

	GLX::AddAbsolute(container, zone);

	GLX::SetOnStyle(container, {}, [zone](const GLX::Style & style)
	{
		zone->SetStyle(style["Zone"]);
	});

	return container;
});
