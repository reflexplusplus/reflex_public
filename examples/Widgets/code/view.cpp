#include "view.h"
#include "widgets/widgets.h"




namespace Widgets { namespace {

struct ViewImpl : View
{
	static constexpr UInt16 kChunkVersion = 1;

	ViewImpl(App & app)
		: View(app, kChunkVersion, L":res:Widgets/styles.glx", true)
		, m_split(New<GLX::SplitBehaviour>())
	{
		SetDelegate("split", m_split);

		GLX::SetFlow(*this, GLX::kFlowX | GLX::kFlowInvert);	//flip so description stays same size on resize

		REFLEX_LOOP(idx, kNumDemo)
		{
			auto panel = New<GLX::Object>();

			auto desc = g_demos[idx];

			auto content = desc->ctr();

			Bootstrap::SetStyleSheet(content, Join(L":res:Widgets/", ToWString(desc->id), L".glx"));

			if (desc->stretch)
			{
				GLX::AddStretch(panel, content);
			}
			else
			{
				GLX::AddFloat(panel, content, GLX::kAlignmentCenter);
			}

			m_tabgroup.AddPanel(desc->name, panel);
		}

		GLX::SetFlow(m_description, GLX::kFlowY);

		Data::SetBool(m_description, GLX::kresizable, true);

		GLX::AddInline(*this, m_description);

		GLX::AddInlineFlex(*this, m_tabgroup);
				
		m_tabgroup.GetSelector()->SelectPanel(0);
	}


	void OnResetState(Key32) override 
	{
		m_tabgroup.GetSelector()->SelectPanel(0); 

		m_split->SetSplitSize(m_description, 256.0f);
	}
	
	void OnRestoreState(Data::Archive::View & stream, Key32) override
	{
		m_tabgroup.GetSelector()->SelectPanel(Data::Deserialize<UInt16>(stream));

		m_split->SetSplitSize(m_description, Data::Deserialize<Float32>(stream));
	}
	
	void OnStoreState(Data::Archive & stream) const override
	{
		Data::Serialize(stream, UInt16(m_tabgroup.GetSelector()->GetCurrentIndex().value));

		Data::Serialize(stream, m_split->GetSplitSize(m_description));
	}


	void OnSetStyle(const GLX::Style & style) override
	{
		m_tabgroup.SetStyle(style["TabGroup"]);

		m_description.SetStyle(style["Description"]);

		Update();
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::Selector::kSelectPanel)
		{
			m_current = g_demos[GLX::GetIndex(e)]->id;

			Update();

			return true;
		}

		return View::OnEvent(src, e);
	}

	void OnUpdate() override
	{
		m_description.Clear();

		auto style = m_description.GetStyle()["Paragraph"];

		for (auto & i : File::EnumerableEmbeddedResource::range)
		{
			if (i.id == Join(L"info/", ToWString(m_current), L".txt"))
			{
				auto binary = File::Extract(i);

				auto stream = ToView(binary);

				WString line;

				while (Data::ReadLine(stream, line))
				{
					if (line.Empty()) continue;

					auto text = New<GLX::Label>(line);

					text->SetStyle(style);

					GLX::AddInline(m_description, text);
				}
			}
		}
	}


	Reference <GLX::SplitBehaviour> m_split;

	GLX::TabGroup m_tabgroup;

	GLX::Object m_description;

	CString::View m_current;
};

} }

Reflex::Unretained <Widgets::View> Widgets::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
