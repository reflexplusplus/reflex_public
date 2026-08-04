#include "../console.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

class ProfileView : public Detail::ConsolePanel
{
public:

	ProfileView();


	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		SelectScope(Data::Deserialize<Key32>(stream));
	}

	void OnStore(Data::Archive & stream) const override
	{
		Data::Serialize(stream, m_current->a->id);
	}




private:

	void Reset();


	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	virtual void OnClock(Float delta) override;

	virtual void OnUpdate() override;


	void SelectScope(Key32 uid);



	State::Monitor m_monitor;


	typedef Pair <Output*,State::Monitor> Item;

	Array <Item> m_scopes;

	Item * m_current;

	Array < Reference <Output::Profiler> > m_profilers;

	Output::Profiler * m_current_item;


	ConstTRef <GLX::Style> m_button_style, m_item_style;

	GLX::Split m_split;	//TODO BEHAVIOUR

	GLX::ListScroller m_list;

	GLX::ScrollerOfType <GLX::Object> m_inspector;

	InfoItem m_min, m_max, m_count;

	GLX::Object m_buttons;

	GLX::Button m_reset;

	GLX::Object m_footer;


	static inline const Key32 kPrefsKey = K32("IDE/Console/Profile");

};




//
//impl

ProfileView::ProfileView()
	: ConsolePanel("Profile", 1)
	, m_monitor(Output::state)
	, m_current_item(nullptr)
	, m_min(L"Min")
	, m_max(L"Max")
	, m_count(L"Count")
	, m_buttons(GLX::kStandardLayoutWrapped)
	, m_reset(L"Reset")
{
	GLX::BindClick(m_reset, Bind(&ProfileView::Reset, this));


	GLX::EnableAutoFit(m_buttons, false, true);

	GLX::SetFlow(m_split, GLX::kFlowY | GLX::kFlowInvert);

	GLX::SetFlow(m_inspector.GetContent(), GLX::kFlowY);

	Data::SetBool(m_inspector, GLX::kresize, true);

	GLX::EnableAutoFit(m_inspector, true, true);


	GLX::AddInlineFlex(m_footer, m_buttons);

	GLX::AddInline(m_footer, m_reset);

	GLX::AddInline(m_split, m_footer);

	auto content = m_inspector.GetContent();

	GLX::AddInline(content, m_min);

	GLX::AddInline(content, m_max);

	GLX::AddInline(content, m_count);

	//AddInline(content, m_average);

	GLX::AddInline(m_split, m_inspector);

	GLX::AddInlineFlex(m_split, m_list);

	GLX::AddStretch(*this, m_split);

	GLX::SetClip(*this, kNullKey);

	EnableOnClock();

	m_scopes.Allocate(8);

	for (auto & i : Output::range)
	{
		auto & item = m_scopes.Push();

		item.a = &i;

		item.b.Connect(i);
	}

	m_current = &m_scopes.GetFirst();
}

void ProfileView::Reset()
{
	for (auto & i : m_profilers)
	{
		i->Reset();
	}
}

void ProfileView::OnSetStyle(const GLX::Style & style)
{
	auto bar = style["Bar"];

	m_button_style = bar["Button"];


	auto list = style["List"];

	m_item_style = style["InfoItem"];

	auto & item = *m_item_style;


	m_list.SetStyle(list);

	m_min.SetStyle(item);

	m_max.SetStyle(item);

	m_count.SetStyle(item);

	//m_average.SetStyle(item);

	m_inspector.SetStyle(list);

	m_reset.SetStyle(m_button_style);

	m_footer.SetStyle(bar);


	Update();
}

bool ProfileView::OnEvent(GLX::Object & source, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		if (source.GetParent() == m_buttons)
		{
			SelectScope(source.id);
		}

		return true;
	}
	else if (e.id == GLX::AbstractList::kListSelect)
	{
		auto idx = GLX::GetIndex(e);

		if (idx < m_profilers.GetSize())
		{
			m_current_item = m_profilers[idx].Adr();

			m_current->b.Reconnect();
		}

		return true;
	}

	return GLX::Object::OnEvent(source, e);
}

void ProfileView::OnClock(Float delta)
{
	if (m_current->b.Poll())
	{
		auto focus = Poll(m_current_item);

		auto content = m_list.GetContent();

		content->Clear();

		for (auto & i : m_profilers)
		{
			UInt precision = i->GetPrecision();

			auto avg = i->GetAverage();

			auto object = Detail::CreateInfoItem(ToWString(i->GetName()), ToWString(avg, precision), false);

			if (auto status = i->GetStatus()) object->SetState(Detail::kItemStates[status]);

			object->SetStyle(*m_item_style);

			if (i.Adr() == focus)
			{
				m_current_item = focus;

				GLX::SetText(m_min.value, ToWString(i->GetMin(), precision));

				GLX::SetText(m_max.value, ToWString(i->GetMax(), precision));

				GLX::SetText(m_count.value, ToWString(i->GetCount()));

				GLX::Select(object);
			}

			AddInline(content, object);
		}
	}
}

void ProfileView::OnUpdate()
{
	m_buttons.Clear();

	for (auto & i : Output::range)
	{
		auto button = AddInline(m_buttons, GLX::Init(REFLEX_CREATE(GLX::Button), *m_button_style, ToWString(i.name)));

		button->id = i.id;

		GLX::Select(button, &i == m_current->a);
	}

	m_profilers.Clear();

	m_current->a->EnumerateProfilers([this](Output::Profiler & item)
	{
		m_profilers.Push(item);
	});
}

void ProfileView::SelectScope(Key32 uid)
{
	for (auto & i : m_scopes)
	{
		auto & scope = *i.a;

		if (uid == scope.id)
		{
			m_current = &i;

			Update();
		}
	}
}

REFLEX_END_INTERNAL;
