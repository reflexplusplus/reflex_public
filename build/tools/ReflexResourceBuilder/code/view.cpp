#include "view.h"




//
//View impl

namespace ResourceBuilder { namespace {	//begin internal namespace

class ViewImpl : public View
{
public:

	ViewImpl(App & app);



private:

	REFLEX_DECLARE_KEY32(RecentV2);


	//bootstrap persistence callbacks
	
	void OnResetState(Key32 context) override;

	void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	void OnStoreState(Data::Archive & stream) const override;



	//glx object callbacks

	void OnSetStyle(const GLX::Style & style) override;

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnClock(Float32 delta) override;

	void OnUpdate() override;



	//helpers

	void OpenXml(const WString & path)
	{
		m_actual_progress = New<Data::Float32Property>();

		m_thread = app->Compile(path, m_actual_progress);

		Remove(m_recent, path);

		m_recent.Insert(0, path);

		StoreRecentList();

		m_time = 0.0f;

		m_faked_progress->value = 0.0f;
	}

	void OpenXml()
	{
		if (auto path = GLX::ShowFileDialog(false, { L"xml" }, System::GetPath(System::kPathDesktop), L"Select XML"))
		{
			OpenXml(path);
		}
	}

	void StoreRecentList()
	{
		m_recent.SetSize(Min<UInt>(m_recent.GetSize(), 25));

		Data::SetWStringArray(Bootstrap::global->prefs, kRecentV2, m_recent);

		Update();
	}



	//links
	
	const TRef <App> app;

	

	//data

	Array <WString> m_recent;



	//elements

	GLX::Button m_open;

	GLX::Object m_header, m_body, m_footer;

	GLX::List m_list;

	GLX::ScrollArea m_scroller;

	GLX::Button m_button;


	Reference <Data::Float32Property> m_actual_progress;
	
	Reference <Data::Float32Property> m_faked_progress = kNewObject;

	Float m_time;

	Reference <System::Task> m_thread;

};

ViewImpl::ViewImpl(App & app)
	: View(app, 0, L":res:ResourceBuilder/styles.txt"),
	app(app),
	m_recent(Data::GetWStringArray(Bootstrap::global->prefs, kRecentV2)),
	m_button(L"Open XML"),
	m_time(1.0f)
{
	Data::SetBool(*this, GLX::kresizable, true);

	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::EnableAutoFit(m_list, false, true);


	GLX::SetText(m_header, L"Resource Builder");


	m_scroller.SetContent(m_list);

	GLX::AddInlineFlex(m_body, m_scroller);

	GLX::AddInline(*this, m_header);

	GLX::AddInlineFlex(*this, m_body);


	GLX::AddInline(*this, m_button);

	GLX::AddInline(*this, m_footer);


	GLX::BeginEventForwarding(*this, GLX::kKeyDown, m_list);
}

void ViewImpl::OnResetState(Key32 context)
{
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	m_header.SetStyle(style["Header"]);

	m_scroller.SetStyle(style["Scroller"]);

	m_button.SetStyle(style["Button"]);

	Update();
}

bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::List::kListSelect)
	{
		return true;
	}
	else if (e.id == GLX::List::kListLoad)
	{
		WString path = m_recent[GLX::GetIndex(e)];

		OpenXml(path);

		return true;
	}
	else if (e.id == GLX::List::kListRequestRemove)
	{
		m_recent.Remove(GLX::GetIndex(e));

		StoreRecentList();

		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (src == m_button)
		{
			OpenXml();
		}
		else if (auto idx = GLX::LookupBranchIndex(m_list, src))
		{
			auto menu = GLX::OpenContextMenu(*this);

			auto path = m_recent[idx.value];

			GLX::BindClick(menu->AddItem(L"Edit"), [path]()
			{
				System::Open(path);
			});

			GLX::BindClick(menu->AddItem(L"Show Location"), [path]()
			{
				System::Open(File::ResolveExistingFolder(File::SplitFilename(path).a));
			});
		}

		return true;
	}
	else if (e.id == GLX::kKeyDown)
	{
		auto key = GLX::GetKeyCode(e);

		if (key == GLX::kKeyCodeNumericPlus)
		{
			OpenXml();

			return true;
		}
	}
	else if (e.id == GLX::kDragDropReceiveExternal)
	{
		if (auto dragfiles = GLX::QueryDragDropData<Data::ArrayOfWStringProperty>(e))
		{
			OpenXml(dragfiles->value.GetFirst());
		}

		return true;
	}
	
	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnClock(Float32 delta)
{
	static constexpr Key32 kProgress = "Progress";
	constexpr Float kMinTime = 0.5f;
	constexpr Float kMaxRisePerSec = 1.0f / kMinTime;

	Bootstrap::View::OnClock(delta);

	m_time += delta;

	bool active = !m_thread->Completed();

	if (active || m_time < kMinTime)
	{
		auto overlay = GLX::Acquire(*this, kProgress, GLX::kEnterAnimationFade, [this]()
		{
			auto overlay = New<GLX::Object>();

			GLX::EnableFloat(overlay, GLX::kOrientationFit, GLX::kOrientationFit);

			overlay->SetProperty(GLX::kdata, m_faked_progress);
	
			overlay->SetStyle(GetStyle()[kProgress]);

			return overlay;
		});

		auto actual = Clip(m_actual_progress->value, 0.0f, 1.0f);

		auto time_gate = Clip(m_time / kMinTime, 0.0f, 1.0f);

		auto target = Min(actual, time_gate);

		auto max_step = kMaxRisePerSec * delta;

		m_faked_progress->value = Min(m_faked_progress->value + max_step, target);

		overlay->Realign();
	}
	else
	{
		m_faked_progress->value = 1.0f;

		GLX::Discard(*this, kProgress);
	}
}

void ViewImpl::OnUpdate()
{
	if (GLX::BranchContains(m_list, GLX::Core::desktop->GetFocus())) Focus();

	m_list.Clear();

	auto style = m_list.GetStyle()["Item"];

	for (auto & i : m_recent)
	{
		GLX::AddInline(m_list, Init(New<GLX::Button>(i), style));
	}
}

} } //end internal namespace

Reflex::TRef <ResourceBuilder::View> ResourceBuilder::View::Create(App & app)
{
	return New<ViewImpl>(app);
}
