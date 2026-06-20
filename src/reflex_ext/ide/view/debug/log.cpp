#include "../console.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

class LogView : public Detail::ConsolePanel
{
public:

	LogView();

	virtual void OnReset(Key32 context) override {}

	void OnStore(Data::Archive & stream) const override
	{
		Data::Serialize(stream, m_filters);
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		m_filters.Clear();

		decltype (m_filters) filters;

		Data::Deserialize(stream, filters);

		for (auto & i : Output::range)
		{
			auto id = i.id;

			if (filters.Search(id)) m_filters.Insert(id);
		}
	}



private:

	void Clear();


	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	virtual void OnClock(Float delta) override;



	State::Monitor m_monitor;


	WString m_unknown;

	Sequence <Key32,WString> m_namespaces;

	Sequence <Key32> m_filters;


	Array <Output::Queue::Type> m_data;


	GLX::VirtualListScroller m_list;

	GLX::Object m_buttons;

	GLX::Button m_clear;

	GLX::Object m_footer;


	ConstTRef <GLX::Style> m_bar_style, m_button_style, m_item_style;


	static inline const Key32 kPrefsKey = K32("IDE/Console/LogV2");

};

LogView::LogView()
	: ConsolePanel("Log", 1),
	m_monitor(Output::state),
	m_unknown(L"[Unknown]"),
	m_buttons(GLX::kStandardLayoutWrapped),
	m_clear(L"Clear")
{
	auto vector = m_list.GetContent();

	vector->SetPopulateCallback([this](UInt start, ArrayRegion < Reference <GLX::Object> > items, const GLX::Style & style)
	{
		auto pitem = m_data.GetData() + start;

		for (auto & object : items)
		{
			auto & item = *pitem++;

			auto & buffer = item.c;

			CString::View ref(buffer.data, buffer.size);

			auto & ns = *m_namespaces.SearchValue(item.a, &m_unknown);

			object = Detail::CreateInfoItem(ns, ToWString(ref), false);

			if (auto type = item.b) object->SetState(Detail::kItemStates[type]);

			GLX::Init(object, m_item_style);
		}
	});


	GLX::BindClick(m_clear, BindMethod(this, &LogView::Clear));


	GLX::EnableAutoFit(m_buttons, false, true);

	GLX::SetFlow(*this, GLX::kFlowY);


	GLX::AddInlineFlex(*this, m_list);

	GLX::AddInlineFlex(m_footer, m_buttons);

	GLX::AddInline(m_footer, m_clear);

	GLX::AddInline(*this, m_footer);


	EnableOnClock();
}

void LogView::Clear()
{
	m_data.Clear();

	m_list.GetContent()->ClearItems();
}

void LogView::OnSetStyle(const GLX::Style & style)
{
	auto bar = style["Bar"];

	m_button_style = bar["Button"];

	auto list = style["List"];

	m_item_style = style["InfoItem"];

	m_list.SetStyle(list);

	m_clear.SetStyle(*m_button_style);

	m_footer.SetStyle(bar);
}

bool LogView::OnEvent(GLX::Object & source, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown && source.GetParent() == m_buttons)
	{
		auto uid = source.id;

		if (auto idx = m_filters.Search(uid))
		{
			m_filters.Remove(idx.value);

			GLX::Select(source, false);
		}
		else
		{
			m_filters.Insert(uid);

			GLX::Select(source);
		}

		return true;
	}
	else if (e.id == GLX::AbstractList::kListSelect)
	{
		GLX::PermitRequest(e, false);

		return true;
	}

	return Object::OnEvent(source, e);
}

void LogView::OnClock(Float delta)
{
	if (m_monitor.Poll())
	{
		m_namespaces.Clear();

		m_buttons.Clear();

		auto filters = std::move(m_filters);

		for (auto & i : Output::range)
		{
			auto button = GLX::AddInline(m_buttons, Init(REFLEX_CREATE(GLX::Button), *m_button_style, ToWString(i.name)));

			auto id = i.id;

			button->id = id;

			if (filters.Search(id))
			{
				m_filters.Insert(id);

				button->SetState(GLX::kSelectedState);
			}

			m_namespaces.Insert(id, ToWString(i.name));
		}
	}

	Output::Queue::Type item;

	bool jump = false;

	if (m_filters)
	{
		while (Output::queue.Pop(item))
		{
			if (m_filters.Search(item.a))
			{
				m_data.Push(item);

				jump = true;
			}
		}
	}
	else
	{
		while (Output::queue.Pop(item))
		{
			m_data.Push(item);

			jump = true;
		}
	}

	UInt n = m_data.GetSize();

	if (n > 512)
	{
		Array <Output::Queue::Type> t;

		t.SetSize(256);

		auto last = m_data.GetData() + n - 1;

		REFLEX_LOOP(idx, 256) Swap(t[idx], *last--);

		m_data.Swap(t);
	}

	if (jump)
	{
		auto vector = m_list.GetContent();

		vector->SetNumItem(m_data.GetSize());

		vector->Reveal(m_data.GetSize() - 1);
	}
}

REFLEX_END_INTERNAL
