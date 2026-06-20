#include "../console.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct AbstractResourcesView : public Detail::ConsolePanel
{
public:

	AbstractResourcesView(Key32 id, UInt16 version);



protected:

	virtual UInt OnUpdate(File::ResourcePool::Lock & lock) = 0;

	virtual void OnPopulate(File::ResourcePool & resourcepool, UInt start, ArrayRegion < Reference <GLX::Object> > items) = 0;

	void Select(File::ResourcePool & resourcepool);


	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnClock(Float) override;

	virtual void OnUpdate() override;

	virtual void OnAttachWindow() override;

	virtual void OnDetachWindow() override;


	File::ResourcePool * m_presourcepool;

	File::ResourcePool::Monitor m_resourcepool_monitor;

	GLX::VirtualListScroller m_list;

	GLX::Object m_footer;

	ConstTRef <GLX::Style> m_item_style;

};

struct AbstractFilesView : public AbstractResourcesView
{
	AbstractFilesView();

	virtual void OnPopulate(File::ResourcePool & resourcepool, UInt start, ArrayRegion < Reference <GLX::Object> > items) override;


	void OnStore(Data::Archive & stream) const override
	{
		Data::Serialize(stream, m_filters);
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		Data::Deserialize(stream, m_filters);
	}

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kMouseDown && src.GetParent() == m_buttons)
		{
			if (auto idx = m_filters.Search(src.id.value))
			{
				m_filters.Remove(idx.value);
			}
			else
			{
				m_filters.Set(src.id.value);
			}

			m_list.GetContent()->Rebuild();

			Update();
		}

		return AbstractResourcesView::OnEvent(src, e);
	}

	Map <Reflex::Detail::DynamicTypeRef> m_types;

	Sequence <TypeID> m_filters;

	Array <const File::ResourcePool::Token*> m_tokens;


	GLX::Object m_buttons;
};

struct FilesView : public AbstractFilesView
{
	virtual UInt OnUpdate(File::ResourcePool::Lock & lock) override;
};

struct FileSystemView : public AbstractResourcesView
{
	FileSystemView()
		: AbstractResourcesView("VirtualFileSystem", 0)
	{
	}
		
	virtual UInt OnUpdate(File::ResourcePool::Lock & lock) override
	{
		m_items.Clear();

		lock.lock.Enumerate([this](File::VirtualFileSystem::Locator & locator)
		{
			m_items.Push(locator);
		});

		return m_items.GetSize();
	}

	virtual void OnPopulate(File::ResourcePool & resourcepool, UInt start, ArrayRegion < Reference <GLX::Object> > items) override;

	virtual void OnRestore(Data::Archive::View & stream, Key32 context) override {}

	virtual void OnStore(Data::Archive & stream) const override {}

	Array < Reference <File::VirtualFileSystem::Locator> > m_items;
};

AbstractResourcesView::AbstractResourcesView(Key32 id, UInt16 version)
	: ConsolePanel(id, version),
	m_presourcepool(TheGlobal::Get()->m_resourcepool.Adr()),
	m_resourcepool_monitor(*m_presourcepool)
{
	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::AddInlineFlex(*this, m_list);

	EnableOnAttachDetachWindow();

	EnableOnClock();
}

void AbstractResourcesView::Select(File::ResourcePool & resourcepool)
{
	m_presourcepool = &resourcepool;

	m_resourcepool_monitor.Connect(resourcepool);

	Update();
}

void AbstractResourcesView::OnSetStyle(const GLX::Style & style)
{
	auto bar = style["Bar"];

	auto list = style["List"];

	m_list.SetStyle(list);

	m_footer.SetStyle(bar);

	m_item_style = style["InfoItem"];
}

bool AbstractResourcesView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::AbstractList::kListSelect)
	{
		GLX::PermitRequest(e, false);

		return true;
	}

	return Detail::ConsolePanel::OnEvent(src, e);
}

void AbstractResourcesView::OnAttachWindow()
{
	if (m_presourcepool)
	{
		Select(*m_presourcepool);
	}
}

void AbstractResourcesView::OnDetachWindow()
{
	m_resourcepool_monitor.Disconnect();
}

void AbstractResourcesView::OnClock(Float)
{
	if (m_resourcepool_monitor.Poll()) Update();
}

void AbstractResourcesView::OnUpdate()
{
	auto vector = m_list.GetContent();

	File::ResourcePool::Lock lock(*m_presourcepool);

	vector->SetNumItem(OnUpdate(lock));

	vector->SetPopulateCallback([this](UInt start, ArrayRegion < Reference <GLX::Object> > items, const GLX::Style & style)
	{
		OnPopulate(*m_presourcepool, start, items);
	});
}

AbstractFilesView::AbstractFilesView()
	: AbstractResourcesView("Resources", 2)
	, m_buttons(GLX::kStandardLayoutWrapped)
{
	GLX::EnableAutoFit(m_buttons, false, true);

	GLX::AddInlineFlex(m_footer, m_buttons);

	GLX::AddInline(*this, m_footer);
}

UInt FilesView::OnUpdate(File::ResourcePool::Lock & lock)
{
	m_types.Clear();

	m_tokens.Clear();

	m_tokens.Allocate(lock.resourcepool->GetTotalNumItem());

	auto filters = std::move(m_filters);

	lock.Enumerate([this, &filters](const File::ResourcePool::Token & token)
	{
		auto type = token.object->object_t;

		m_types.Set(type);

		if (filters.Search(type->type_id)) m_filters.Set(type->type_id);
	});

	lock.Enumerate([this, &filters](const File::ResourcePool::Token & token)
	{
		auto type = token.object->object_t;

		if (!m_filters || m_filters.Search(type->type_id))
		{
			m_tokens.Push(&token);
		}
	});


	auto style = m_footer.GetStyle()["Button"];

	GLX::Detail::Recycler recycler(m_buttons, style);

	for (auto & i : m_types)
	{
		auto type = i.key;

		auto ref = recycler.Acquire(type->type_id, {}, [type]()
		{
			auto btn = New<GLX::Button>(ToWString(CString::View(type->tname)));

			return Cast<GLX::Object>(btn);
		});

		GLX::Select(ref, True(m_filters.Search(type->type_id)));
	}

	return m_tokens.GetSize();
}

void AbstractFilesView::OnPopulate(File::ResourcePool & resourcepool, UInt start, ArrayRegion < Reference <GLX::Object> > items)
{
	auto ptoken = m_tokens.GetData() + start;

	for (auto & i : items)
	{
		auto & token = **ptoken++;

		auto & object = *token.object;

		i = GLX::Init(Detail::CreateInfoItem(ToWString(ToView(object.object_t->tname)), token.path, true), m_item_style);

		i->SetState(Detail::kItemStates[2 - token.attributes.status]);
	}
}

void FileSystemView::OnPopulate(File::ResourcePool & resourcepool, UInt start, ArrayRegion < Reference <GLX::Object> > items)
{
	auto idx = start + items.size;

	auto plocatoref = m_items.GetData() + start + items.size;

	for (auto & i : items)
	{
		auto & locator = **(--plocatoref);

		auto type = ToWString(ToView(locator.object_t->tname));

		i = GLX::Init(Detail::CreateInfoItem(ToWString(idx--), type, false), m_item_style);
	}
}

REFLEX_END_INTERNAL
