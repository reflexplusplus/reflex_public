



//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct MemoryView : public Detail::ConsolePanel
{
	MemoryView();


	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual void OnClock(Float delta) override;

	virtual void OnRestore(Data::Archive::View & stream, Key32 context) override {}

	virtual void OnStore(Data::Archive & stream) const override {}



	UInt m_nallocations, m_nbytes;


	InfoItem m_usage, m_allocations;

	GLX::VirtualListScroller m_list;

	ConstTRef <GLX::Style> m_item_style;


	Float32 m_remainder;
};

MemoryView::MemoryView()
	: ConsolePanel(K32("MemoryView"), 0),
	m_nallocations(0),
	m_nbytes(0),
	m_usage(L"Usage"),
	m_allocations(L"Allocations"),
	m_remainder(0.0f)
{
	GLX::SetFlow(*this, GLX::kFlowY);

	
	GLX::AddInline(*this, m_usage);

	GLX::AddInline(*this, m_allocations);

	GLX::AddInlineFlex(*this, m_list);


	EnableOnClock();
}

void MemoryView::OnSetStyle(const GLX::Style & style)
{
	auto list = style["List"];

	m_list.SetStyle(list);

	m_item_style = style["InfoItem"];

	m_usage.SetStyle(m_item_style);

	m_allocations.SetStyle(m_item_style);
}

void MemoryView::OnClock(Float delta)
{
	m_remainder -= delta;

	if (m_remainder <= 0.0f)
	{
		m_remainder += 0.25f;

		StandardAllocator::Lock lock(g_default_allocator);

		if (SetFiltered(m_nbytes, UInt(lock.allocator->GetNumBytes())))
		{
			Float64 nbyte = m_nbytes;

			GLX::SetText(m_usage.value, Join(ToWString(nbyte / (1024.0 * 1024.0), 2), L"mb"));
		}

		if (SetFiltered(m_nallocations, lock.allocator->GetNumAllocation()))
		{
			GLX::SetText(m_allocations.value, ToWString(m_nallocations));

			auto vector = m_list.GetContent();

			vector->SetNumItem(m_nallocations);
		}
	}
}

REFLEX_END_INTERNAL
