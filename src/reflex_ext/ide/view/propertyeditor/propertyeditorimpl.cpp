#include "propertyeditorimpl.h"




//
//view

REFLEX_BEGIN_INTERNAL(Reflex::IDE::Detail)

struct PropertyEditorImpl::Dialog : public GLX::Object
{
public:

	REFLEX_DECLARE_KEY32(PropertyEditor);


	struct Field;

	static void Create(PropertyEditorImpl & view, Object & owner, const WString::View & label, const Array <Field*> & fields, const Function <void(const Array<Field*>&)> & ondone, UInt startidx = 0);

	Dialog(Object & owner, const WString::View & label, const Array<Field*> & fields, const Function <void(const Array<Field*>&)> & ondone, UInt startidx);



private:

	void Submit(bool close = true);

	bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	void OnClock(Float) override;

	void OnAttachWindow() override;

	void OnDetachWindow() override;

	void OnSetStyle(const GLX::Style & style) override;

	bool OnDragDropReceiveExternal(const Reflex::Object & object) override;



	Object & owner;

	//const GLX::Style & field_style;


	Array <Field*> m_fields;

	UInt m_startidx;

	Function <void(const Array<Field*>&)> m_ondone;

	GLX::Object m_footer;

	GLX::Object m_flex;

	GLX::Button m_ok, m_cancel;
};

struct PropertyEditorImpl::Dialog::Field : public GLX::Object
{
public:

	Field(PropertyEditorImpl & view, const Interface::PropertyRef & propertyref);

	void Apply();


	GLX::TextArea & GetTextEdit() { return m_textedit; }



private:

	friend Dialog;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual void OnUpdate() override;



	PropertyEditorImpl & view;

	Interface::PropertyRef m_property;


	GLX::TextArea m_textedit;

	GLX::Object m_popup;

	bool m_edited = false;

};

struct PropertyEditorImpl::ModificationScope
{
	ModificationScope(View & view);

	~ModificationScope();

	View & view;

	const Interface & iface;

	Data::PropertySet & data;

	static inline UInt g_n = 0;
};

struct PropertyEditorImpl::Transaction : public ModificationScope
{
	Transaction(View & view);

	~Transaction();

	Data::Archive m_undo, m_redo;
};

struct PropertyEditorImpl::Attribute : public GLX::Object
{
	Attribute(View & view);

	void Set(const Interface::PropertyRef & propertyref);

	virtual void OnSetStyle(const GLX::Style & style) override;

	
	View & view;

	Interface::PropertyRef propertyref;

	const Data::Detail::StandardPropertySheetInterface::TypeInfo * pvaluetype;

	bool isarray;

	GLX::Object key, value, type;
};

PropertyEditorImpl::PropertyEditorImpl(const Data::Detail::StandardPropertySheetInterface & propertysheet_interface, const Interface & iface)
	: PropertyEditor(iface),
	m_propertysheet_interface(propertysheet_interface),
	m_typeinfo(iface.GetObjectType().a),
	m_null(iface.GetObjectType().b),
	m_keymap(iface.GetKeyMap()),
	m_focus(m_typeinfo, m_null),
	styles(ide_styles["PropertyEditor"]),
	m_mute(0)
{
	Data::iStreamable::Publish(*this);
		
	Retain(iface);
	
	Retain(ide_styles);

	//if (!kTypeNames[0].a.b)
	//{
	//	kTypeNames[Interface::kTypeUInt8] = { "U8", L"UInt8" };
	//	kTypeNames[Interface::kTypeUInt16] = { "U16", L"UInt16" };
	//	kTypeNames[Interface::kTypeUInt32] = { "U32", L"UInt32" };
	//	kTypeNames[Interface::kTypeUInt64] = { "U64", L"UInt64" };
	//	kTypeNames[Interface::kTypeFloat32] = { "F32", L"Float32" };
	//	kTypeNames[Interface::kTypeBinary] = { "Bin", L"Binary" };

	//	kTypeNames[Interface::kTypeKey32] = { "K32", L"Key32" };
	//	kTypeNames[Interface::kTypeCString] = { "CStr", L"CString" };
	//}


	SetStyle(styles);

	auto bar = styles[K32("Bar")];

	m_styles.nodes = styles[K32("Nodes")];

	m_styles.properties = styles[K32("Properties")];

	m_styles.attribute = m_styles.properties[GLX::kitem];

	m_styles.button = bar[K32("Button")];


	//m_nodes_bar.SetStyle(bar);

	m_tree.SetStyle(*m_styles.nodes);

	m_properties_bar.SetStyle(bar);

	m_properties.SetStyle(*m_styles.properties);

	//Init(m_root, m_styles.button, L"Root");

	//Init(m_add_child, m_styles.button, L"Add");

	//Init(m_delete, m_styles.button, L"Remove");


	GLX::Init(m_add_property, m_styles.button, L"Add");

	GLX::Init(m_remove_property, m_styles.button, L"Remove");


	Data::SetBool(m_tree, GLX::kWantsFocus, true);

	Data::SetBool(m_properties, GLX::kWantsFocus, true);

	GLX::SetFlow(m_properties_area, GLX::kFlowY);

	GLX::EnableFocusCycle(*this);



	if (auto groups = iface.GetPropertyGroups())
	{
		GLX::AddInlineFlex(m_properties_bar, m_property_groups);

		for (auto & i : groups)
		{
			GLX::AddInline(m_property_groups, GLX::Init(New<GLX::Button>(i), m_styles.button))->id = i;
		}
		
		m_property_group = m_property_groups.GetFirst()->id;
	}

	GLX::AddInline(m_properties_area, m_properties_bar);

	GLX::AddInlineFlex(m_properties_area, m_properties);

	GLX::SetFlow(*this, GLX::kFlowY | GLX::kFlowInvert);
	Data::SetBool(m_properties_area, GLX::kresize, true);
	SetSplitSize(m_properties_area, 192.0f);
	GLX::AddInline(*this, m_properties_area);
	GLX::AddInlineFlex(*this, m_tree);

	EnableOnAttachDetachWindow();
}

PropertyEditorImpl::~PropertyEditorImpl()
{
	Release(ide_styles);
	
	Release(interface);
}

void PropertyEditorImpl::Clear()
{
	SetRoot(m_null);
}

void PropertyEditorImpl::SetRoot(Data::PropertySet & root)
{
	bool valid = root.GetAllocator() || root == m_null;

	REFLEX_ASSERT(valid);

	if (m_root.Adr() != &root && valid)
	{
		m_root = root;

		PropertyEditorImpl::ModificationScope scope(*this);

		Open(m_root);

		PropertyEditorImpl::SetFocus(*this, root);

		Associate(m_tree.root, root);
	}
}

TRef <Data::PropertySet> PropertyEditorImpl::GetRoot()
{
	return m_root;
}

void PropertyEditorImpl::SetFocus(Data::PropertySet & node)
{
	if (GetParent(node))
	{
		if (PropertyEditorImpl::SetFocus(*this, node))
		{
			ComputeLayout();

			Visit(m_tree.root, [this, pfocus = m_focus.Load().Adr()](GLX::Tree::Node & node)
			{
				if (GetData(node) == pfocus)
				{
					node.Reveal();
				}
			});
		}
	}
	else
	{ 
		PropertyEditorImpl::SetFocus(*this, m_root);
	}
}

TRef <Data::PropertySet> PropertyEditorImpl::GetFocus()
{
	return m_focus.Load();
}

void PropertyEditorImpl::Open(Data::PropertySet & node)
{
	Data::SetBool(node, PropertyEditorImpl::kOpen, true);
}

void PropertyEditorImpl::Close(Data::PropertySet & node)
{
	Data::UnsetBool(node, PropertyEditorImpl::kOpen);
}

bool PropertyEditorImpl::IsOpen(const Data::PropertySet & node)
{
	return Data::GetBool(node, PropertyEditorImpl::kOpen);
}

void PropertyEditorImpl::OnStore(Data::Archive & stream) const
{
	Data::Serialize(stream, GetSplitSize(m_properties_area));
}

void PropertyEditorImpl::OnRestore(Data::Archive::View & stream, Key32 context)
{
	SetSplitSize(m_properties_area, Data::Deserialize<Float32>(stream));
}

bool PropertyEditorImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	typedef GLX::Tree Tree;

	typedef GLX::AbstractList AbstractList;

	constexpr auto IsNodeEvent = [](PropertyEditorImpl * self, GLX::Object & src, GLX::Event & e, Key32 id) -> Data::PropertySet *
	{
		if (e.id == id)
		{
			return self->GetData(Cast<GLX::Tree::Node>(src));
		}

		return 0;
	};

	if (Split::OnEvent(src, e))
	{
		return true;
	}
	else
	{
		if (auto data = IsNodeEvent(this, src, e, Tree::kNodeOpen))
		{
			if (m_mute) return true;

			if (auto children = GetChildren(*data))
			{
				//GLX::AllowEvent(e, children.GetFirst().b.Adr());

				Open(*data);

				GLX::Emit(*this, kEditNode, knode, *data);

				auto ui = Cast<GLX::Tree::Node>(src);

				PropertyEditorImpl::RefreshBranch(*this, *data, ui);
			}

			return true;
		}
		else if (auto data = IsNodeEvent(this, src, e, Tree::kNodeClose))
		{
			if (m_mute) return true;

			Close(*data);

			GLX::Emit(*this, kEditNode, knode, *data);

			return true;
		}
		else if (auto data = IsNodeEvent(this, src, e, Tree::kNodeSelect))
		{
			if (m_mute) return true;

			PropertyEditorImpl::SetFocus(*this, *data);

			return true;
		}
		else if (auto data = IsNodeEvent(this, src, e, Tree::kNodeDeselect))
		{
			if (m_mute) return true;

			if (m_focus.Load().Adr() == data) PropertyEditorImpl::SetFocus(*this, m_root);

			return true;
		}
		else if (auto data = IsNodeEvent(this, src, e, Tree::kNodeRemove))
		{
			PropertyEditorImpl::RemoveNode(*this, *data);

			GLX::PermitRequest(e, false);

			return true;
		}
		else if (e.id == Tree::kNodeRequestInsert)
		{
			PropertyEditorImpl::Transaction t(*this);

			auto a = Cast<Tree::Node>(src);

			auto b = GetData(*e.QueryProperty<Tree::Node>(K32("source")));

			if (a && b)
			{
				PropertyEditorImpl::Insert(*this, *a, *b, Data::GetBool(e, K32("after")));
			}

			return true;
		}
		else if (e.id == AbstractList::kListSelect)
		{
			return true;
		}
		else if (e.id == AbstractList::kListLoad)
		{
			auto properties = m_properties.GetContent();

			auto item = Cast<PropertyEditorImpl::Attribute>(GLX::LookupChildAtIndex(*properties, GetIndex(e)));

			BeginEditProperty(item->propertyref.a);

			return true;
		}
		else if (e.id == AbstractList::kListRequestRemove)
		{
			PropertyEditorImpl::RemoveSelectedProperties(*this);

			return true;
		}
		else if (e.id == GLX::kMouseDown)
		{
			if (GLX::GetClickFlags(e) & GLX::kClickFlagRmb)
			{
				if (GLX::BranchContains(m_tree, src))
				{
					if (auto node = DynamicCast<Tree::Node>(src.GetParent()))
					{
						auto menu = OpenContextMenu(m_tree);

						node->Select();

						auto focus = ByRef(*m_focus.Load().Adr());

						GLX::BindClick(menu->AddItem(L"Send Top"), Bind(&PropertyEditorImpl::Move<false, true>, ByRef(*this), focus));

						GLX::BindClick(menu->AddItem(L"Send Bottom"), Bind(&PropertyEditorImpl::Move<true, true>, ByRef(*this), focus));

						menu->AddSeparator();

						//GLX::BindClick(menu->AddItem(L"Duplicate"), Bind(&PropertyEditorImpl::DuplicateNode, ByRef(*this), focus));

						//GLX::BindClick(menu->AddItem(L"Copy"), Bind(&PropertyEditorImpl::CopyNode, ByRef(*this), focus));

						//auto paste = GLX::BindClick(menu->AddItem(L"Paste"), Bind(&PropertyEditorImpl::PasteNode, ByRef(*this), focus));

						//GLX::Activate(paste, QueryProperty<Data::Archive>(this, K32("Copy")));

						//menu->AddSeparator();

						GLX::BindClick(menu->AddItem(L"Remove"), Bind(&PropertyEditorImpl::RemoveNode, ByRef(*this), focus));
					}
				}
			}
			else if (src.GetParent() == m_property_groups)
			{
				m_property_group = src.id;

				m_animate = true;

				Update();
			}
			else if (src == m_add_property)
			{
				PropertyEditorImpl::BeginAddProperty(*this);
			}
			else if (src == m_remove_property)
			{
				PropertyEditorImpl::RemoveSelectedProperties(*this);
			}

			return true;
		}
		else if (e.id == GLX::kKeyDown)
		{
			auto keycode = GLX::GetKeyCode(e);

			if (keycode == GLX::kKeyCodeNumericPlus)
			{
				if (BranchContains<GLX::Object>(m_properties_bar, src))
				{
					PropertyEditorImpl::BeginAddProperty(*this);
				}
				else if (BranchContains<GLX::Object>(m_properties, src))
				{
					PropertyEditorImpl::BeginAddProperty(*this);
				}
				//else if (m_focus == m_root)
				//{
				//	PropertyEditorImpl::BeginAddNode(*this, m_focus);
				//}
				//else if (auto parents = PropertyEditorImpl::GetParents(*this, m_focus))
				//{
				//	PropertyEditorImpl::BeginAddNode(*this, *parents.GetFirst());
				//}

				Data::SetUInt8(e, GLX::kkeycode, GLX::kKeyCodeNull); //prevent parents from using key

				return false;	//stop + character
			}
			else if (GLX::GetModifierKeys(e) & GLX::kModifierKeyPrimary)
			{
				if (keycode == GLX::kKeyCodeZ)
				{
					Undo();

					return true;
				}
				else if (keycode == GLX::kKeyCodeY)
				{
					Redo();

					return true;
				}
			}
		}

		return false;
	}
}

void PropertyEditorImpl::OnAttachWindow()
{
	GLX::AttachPeriodicClock(*this, K32("clock"), 0.1f, BindMethod(this, &PropertyEditorImpl::PopulateProperties));
}

void PropertyEditorImpl::OnDetachWindow()
{
	m_focus.Store(m_null);

	GLX::DetachClock(*this, K32("clock"));
}

void PropertyEditorImpl::OnUpdate()
{
	GLX::AnimationScope scope(SetFiltered(m_animate, false));

	if (!GetParent(m_focus.Load()))
	{
		PropertyEditorImpl::SetFocus(*this, m_root);
	}

	PropertyEditorImpl::RefreshBranch(*this, m_root, m_tree.root);

	PropertyEditorImpl::PopulateProperties();

	m_tree.Accommodate();
}

void PropertyEditorImpl::OnRestoreHistory(Data::Archive::View stream, bool redo)
{
	Data::Archive ur[2];

	Data::Deserialize(stream, ur);

	auto decoder = ToView(ur[redo]);

	interface->Deserialize(m_root, decoder);
}

const Key32 PropertyEditorImpl::PropertyEditorImpl::kLeafState = "leaf";

bool PropertyEditorImpl::PropertyEditorImpl::SetFocus(PropertyEditorImpl & view, Data::PropertySet & node)
{
	if (view.m_focus.Store(node))
	{
		Array <Data::PropertySet*> parents;

		view.GetParents(node, parents);

		for (auto pparent : parents)
		{
			view.Open(*pparent);
		}

		view.Update();

		GLX::Emit(view, kSelectNode, knode, node);

		return true;
	}

	return false;
}

Data::PropertySet * PropertyEditorImpl::GetParent(Data::PropertySet & node)
{
	Array <Data::PropertySet*> output;

	GetParents(node, output);

	return output ? output.GetLast() : 0;
}

void PropertyEditorImpl::GetParents(Data::PropertySet & node, Array <Data::PropertySet*> & output)
{
	Function <bool(const Interface &, Data::PropertySet &, Data::PropertySet &, Array <Data::PropertySet*> &)> BranchContains;

	REFLEX_LOCAL(bool,Collect)(const Interface & iface, Data::PropertySet & root, Data::PropertySet & node, Array <Data::PropertySet*> & parents)
	{
		Interface::Children children;

		iface.GetChildren(root, children);

		for (auto & i : children)
		{
			Data::PropertySet & child = i.b;

			if (child == node)
			{
				REFLEX_ASSERT(!Search(parents, &root));

				parents.Push(&root);

				return true;
			}

			if (Call(iface, child, node, parents))
			{
				REFLEX_ASSERT(!Search(parents, &root));

				parents.Push(&root);

				return true;
			}
		}

		return false;
	}
	REFLEX_END

	auto root = GetRoot();

	Collect::Call(interface, root, node, output);
}

Data::PropertySet * PropertyEditorImpl::PropertyEditorImpl::GetPrev(Data::PropertySet & node)
{
	if (auto parent = GetParent(node))
	{
		Data::PropertySet * prev = 0;

		for (auto & i : GetChildren(*parent))
		{
			Data::PropertySet & child = i.b;

			if (child == node) return prev;

			prev = &child;
		}
	}

	return 0;
}

Data::PropertySet * PropertyEditorImpl::PropertyEditorImpl::GetNext(Data::PropertySet & node)
{
	if (auto parent = GetParent(node))
	{
		auto children = GetChildren(*parent);

		Data::PropertySet * next = 0;

		REFLEX_RLOOP_PTR(children.GetData(), itr, children.GetSize())
		{
			Data::PropertySet & child = itr->b;

			if (child == node) return next;

			next = &child;
		}
	}

	return 0;
}

bool PropertyEditorImpl::PropertyEditorImpl::BeginAddNode(View & view, Data::PropertySet & parent)
{
	typedef Dialog::Field Field;

	//constexpr auto OnDone = [](const Array <Dialog::Field*> & fields, PropertyEditorImpl & view, Data::PropertySet & parent)
	//{
	//	Transaction t(view);

	//	auto & iface = *view.interface;

	//	if (auto child = AddChild(view, parent))
	//	{
	//		auto & type = *fields[0];

	//		iface.SetProperty(*child, type.GetType(), kType, type.GetValue());


	//		auto & id = *fields[1];

	//		iface.SetProperty(*child, id.GetType(), kID, id.GetValue());


	//		SetFocus(view, *child);

	//		view.Open(parent);
	//	}
	//};

	//Array <Field*> fields;

	//fields.Push(REFLEX_CREATE(Field, view, L"type", Data::Archive::View(), Interface::kTypeCString, false, false));

	//fields.Push(REFLEX_CREATE(Field, view, L"id", Data::Archive::View(), Interface::kTypeCString, true, false));

	//Dialog::Create(view, view.m_tree, view.styles["Dialog"], L"Add Node", fields, Bind(&OnDone::Call, _P1, ByRef(view), ByRef(parent)));

	return true;
}

void PropertyEditorImpl::PropertyEditorImpl::RemoveNode(View & view, Data::PropertySet & node)
{
	if (auto parent = view.GetParent(node))
	{
		Transaction t(view);

		if (auto next = view.GetNext(node))
		{
			SetFocus(view, *next);
		}
		else if (auto prev = view.GetPrev(node))
		{
			SetFocus(view, *prev);
		}
		else
		{
			SetFocus(view, *parent);
		}

		RemoveChild(view, *parent, node);
	}
}

template <bool FWD, bool END> void PropertyEditorImpl::Move(View & view, Data::PropertySet & node)
{
	if (view.GetParent(node))
	{
		Transaction t(view);

		auto next = FWD ? view.GetNext(node) : view.GetPrev(node);

		if (next)
		{
			while (END)
			{
				auto itr = FWD ? view.GetNext(*next) : view.GetPrev(*next);

				if (!itr) break;

				next = itr;
			}

			Insert(view, node, *next, FWD);
		}
	}
}

//void PropertyEditorImpl::CopyNode(View & view, Data::PropertySet & node)
//{
//	if (auto propertyset = DynamicCast<File::PropertySet>(node))
//	{
//		view.SetProperty(K32("Copy"), Data::ToBinary(*propertyset));
//	}
//}
//
//void PropertyEditorImpl::PasteNode(View & view, Data::PropertySet & node)
//{
//	if (auto data = Data::GetBinary(view, K32("Copy")))
//	{
//		if (auto propertyset = DynamicCast<File::PropertySet>(node))
//		{
//			Data::Deserialize(data, *propertyset);
//		}
//	}
//}

//void PropertyEditorImpl::DuplicateNode(View & view, Data::PropertySet & node)
//{
////	constexpr auto Clone = [](View & view, Data::PropertySet & src, Data::PropertySet & dest)
////	{
////		auto & iface = *view.m_interface;
////
////		REFLEX_FOREACH(itr, iface.GetPro(src)) iface.SetAttribute(dest, itr.a, itr.b, itr.c);
////
////		for (auto & i : iface.GetChildren(src))
////		{
////			if (auto dest_child = AddChild(view, dest)) Call(view, i.c, *dest_child);
////		}
////	};
//
//	Transaction t(view);
//
//	if (auto parent = view.GetParent(node))
//	{
//		if (auto child = AddChild(view, *parent))
//		{
//			Insert(view, *child, node, true);
//
//			//Clone::Call(view, node, *child);
//		}
//	}
//}

void PropertyEditorImpl::BeginAddProperty(View & view)
{
	//constexpr auto OnDone = [](const Array <Dialog::Field*> & fields, PropertyEditorImpl & view)
	//{
	//	if (auto key = Data::Unpack<CString::View>(fields[0]->GetValue()))
	//	{
	//		Transaction t(view);

	//		auto & value = *fields[1];

	//		view.interface->SetProperty(view.m_focus, value.GetType(), key, value.GetValue());
	//	}
	//};

	////if (view.m_edit_properties)
	//{
	//	typedef Dialog::Field Field;

	//	Array <Field*> fields;

	//	fields.Push(REFLEX_CREATE(Field, view, L"Key", Data::Archive::View(), Interface::kTypeCString, false, false));

	//	fields.Push(REFLEX_CREATE(Field, view, L"Value", Data::Archive::View(), Interface::kTypeCString, true, true));

	//	Dialog::Create(view, view.m_properties, view.styles["Dialog"], L"Add Property", fields, Bind(&OnDone::Call, _P1, ByRef(view)));
	//}
}

void PropertyEditorImpl::BeginEditProperty(Address address)
{
	//constexpr auto OnDone = [](const Array <Dialog::Field*> &fields, PropertyEditorImpl & view, Address previous)
	//{
	//	Transaction t(view);

	//	auto & iface = *view.interface;

	//	iface.RemoveProperty(view.m_focus, previous);

	//	auto & key = *fields[0];

	//	auto & value = *fields[1];

	//	iface.SetProperty(view.m_focus, value.GetType(), Data::Unpack<CString::View>(key.GetValue()), value.GetValue());
	//};

	//auto & iface = *interface;

	//Interface::Properties properties;

	//iface.GetProperties(m_focus, m_property_group, properties);

	//if (auto pproperty = SearchValue<KeyCompare>(properties, address))
	//{
	//	//Data::Archive archive = GetAttribute2(iface.GetProperties(view.m_focus, view.m_property_group), type, key);

	//	Array <Dialog::Field*> fields;

	//	fields.Push(REFLEX_CREATE(Dialog::Field, *this, L"Key", Data::GetKey(m_keymap, address.id), Interface::kTypeCString, false, false));

	//	//fields.Push(REFLEX_CREATE(Field, *this, L"Value", archive, type, true, true));

	//	Dialog::Create(*this, view.m_properties, view.styles["Dialog"], L"Edit Property", fields, Bind(&OnDone::Call, _P1, ByRef(this), previous), 1);
	//}
}

void PropertyEditorImpl::RemoveSelectedProperties(View & view)
{
	view.m_animate = true;

	auto & iface = *view.interface;

	Transaction t(view);

	auto properties = view.m_properties.GetContent();

	auto selection = GLX::GetListSelection(properties);

	REFLEX_RFOREACH(x, selection)
	{
		auto item = Cast<Attribute>(GLX::LookupChildAtIndex(*properties, x));

		iface.RemoveProperty(view.m_focus.Load(), item->propertyref.a);

		GLX::SkipEnter(item);

		GLX::Exit(item, true);
	}
}

void PropertyEditorImpl::RefreshBranch(View & view, Data::PropertySet & data, UI & ui)
{
	view.m_mute++;

	view.m_tree.SelectNone();

	view.RefreshNode(data, ui, L"");

	Visit(view.m_tree.root, [&view, pfocus = view.m_focus.Load().Adr()](GLX::Tree::Node & node)
	{
		if (view.GetData(node) == pfocus)
		{
			node.Select();
		}
	});

	view.m_mute--;
}

void PropertyEditorImpl::RefreshNode(Data::PropertySet & node, UI & ui, const WString::View & id, bool animate)
{
	SetText(ui.header, ToWString(ToView(node.object_t->tname)), kTypeLabelID);

	SetText(ui.header, id);
	
	auto children = GetChildren(node);
	
	GLX::SetState(ui, kLeafState, children.Empty());

	if (IsOpen(node))
	{
		Array < Reference <GLX::Object> > recycle;

		REFLEX_RFOREACH(i, *ui.body)
		{
			recycle.Push(i);
		}

		for (auto & i : children)
		{
			Data::PropertySet & child = i.b;

			REFLEX_RLOOP(idx, recycle.GetSize())
			{
				auto & r = recycle[idx];

				if (GetData(r) == &child)
				{
					RefreshNode(child, Cast<GLX::Tree::Node>(r), i.a, animate);

					r->SendTop();

					recycle.Remove(idx);

					goto NextChild;
				}
			}

			{
				auto childui = ui.AddNode();

				Associate(childui, child);

				RefreshNode(child, childui, i.a, animate);
			}

			REFLEX_MARKER(NextChild);
		}

		for (auto & i : recycle)
		{
			Deassociate(i);

			Exit(i, true);
		}

		ui.Open(animate);
	}
	else
	{
		ui.Close(animate);
	}
}

void PropertyEditorImpl::PopulateProperties()
{
	m_keymap = interface->GetKeyMap();
		
	GLX::SelectChildren(m_property_groups, false);
	
	GLX::Select(*GLX::QueryChildById(m_property_groups, m_property_group));
	
	auto & iface = *interface;

	auto list = m_properties.GetContent();

	GLX::Detail::Recycler recycler(list, m_styles.attribute, [this]()
	{
		return Cast<GLX::Object>(REFLEX_CREATE(Attribute, *this));
	});

	Interface::Properties properties;

	iface.GetProperties(m_focus.Load(), m_property_group, properties);

	for (auto & i : properties)
	{
		if (i.a.id != Reflex::Detail::AbstractWeakRef::kWeakReferences)
		{
			auto ui = Cast<Attribute>(recycler.Acquire(i.a.id));

			ui->Set(i);

			//ui->Update();
		}
	}
}

PropertyEditorImpl::Dialog::Field::Field(PropertyEditorImpl & view, const Interface::PropertyRef & propertyref)
	: view(view),
	m_textedit(false)
{
	m_popup.SetDelegate({}, New<GLX::PopupBehaviour>());

	Data::SetBool(m_textedit, GLX::kWantsFocus, true);

	GLX::EnableAutoFit(m_textedit, false, false);

	GLX::AddInlineFlex(*this, m_textedit);

	Update();
}

void PropertyEditorImpl::Dialog::Field::Apply()
{
	//if (SetFiltered(m_edited, false)) m_value = StringToValue(view, GLX::GetText(m_textedit.GetContent()), m_type);
}

bool PropertyEditorImpl::Dialog::Field::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kTransaction)
	{
		m_edited = true;

		if (GLX::GetTransactionStage(e) == GLX::kTransactionEnd)
		{
			Apply();

			Update();
		}

		return true;
	}

	return false;
}

void PropertyEditorImpl::Dialog::OnSetStyle(const GLX::Style & style)
{
	auto button = style["Button"];

	m_ok.SetStyle(button);

	m_cancel.SetStyle(button);

	auto field_style = style["Field"];

	for (auto & i : m_fields) i->SetStyle(field_style);
}

void PropertyEditorImpl::Dialog::Field::OnSetStyle(const GLX::Style & style)
{
	auto id = GetProperty<GLX::Text>(m_textedit.GetContent(), GLX::kdata)->IsMultiLine() ? K32("MultiText") : K32("Text");

	m_textedit.SetStyle(style[id]);

	m_popup.SetStyle(style["Type"]);
}

void PropertyEditorImpl::Dialog::Field::OnUpdate()
{
	//CString string = ValueToString(view, m_type, m_value, true);

	//GLX::SetText(m_textedit, ToWString(string));

	//GLX::SetText(m_popup, kTypeNames[m_type].b);
}

void PropertyEditorImpl::Dialog::Create(PropertyEditorImpl & view, GLX::Object & owner, const WString::View & label, const Array <Field*> & fields, const Function <void(const Array<Field*>&)> & ondone, UInt startidx)
{
	DiscardOverlay(view, kPropertyEditor);

	AcquireOverlay(view, kPropertyEditor, false, false, K32("Modal"), [&owner, &label, &fields, &ondone, startidx](GLX::Object & overlay)
	{
		auto dialog = REFLEX_CREATE(Dialog, owner, label, fields, ondone, startidx);

		GLX::AddFloat(overlay, dialog, GLX::kOrientationFit, GLX::kOrientationCenter);

		GLX::SetOnStyle(overlay, [dialog](const GLX::Style & style)
		{
			dialog->SetStyle(style["Dialog"]);
		});
	});
}

PropertyEditorImpl::Dialog::Dialog(GLX::Object & owner, const WString::View & label, const Array <Field*> & fields, const Function <void(const Array<Field*>&)> & ondone, UInt startidx)
	: owner(owner),
	//field_style(style["Field"]),
	m_fields(fields),
	m_startidx(startidx),
	m_ondone(ondone)
{
	GLX::SetText(*this, label);


	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::SetText(m_ok, L"OK");

	GLX::SetText(m_cancel, L"Cancel");


	for (auto & i : m_fields) GLX::AddInline(*this, i);

	GLX::AddInline(m_footer, m_ok);

	GLX::AddInlineFlex(m_footer, m_flex);

	GLX::AddInline(m_footer, m_cancel);

	GLX::AddInline(*this, m_footer);


	GLX::EnableFocusCycle(*this);

	EnableOnAttachDetachWindow();
}

void PropertyEditorImpl::Dialog::Submit(bool close)
{
	for (auto & i : m_fields) i->Apply();

	m_ondone(m_fields);

	if (close) GLX::EmitCloseRequest(*this);
}

bool PropertyEditorImpl::Dialog::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		if (src == m_ok)
		{
			Submit();
		}
		else if (src == m_cancel)
		{
			GLX::EmitCloseRequest(*this);
		}

		return true;
	}
	else if (e.id == GLX::kComplete)
	{
		Submit();

		return true;
	}
	else if (e.id == GLX::kKeyDown)
	{
		auto keycode = GLX::GetKeyCode(e);

		if (keycode == GLX::kKeyCodeF5)
		{
			Submit(false);

			return true;
		}
	}

	return false;
}

void PropertyEditorImpl::Dialog::OnAttachWindow()
{
	EnableOnClock();
}

void PropertyEditorImpl::Dialog::OnClock(Float)
{
	EnableOnClock(false);

	Field & field = *m_fields[m_startidx];

	field.ComputeLayout();

	field.GetTextEdit().GetContent()->Focus();
}

void PropertyEditorImpl::Dialog::OnDetachWindow()
{
	owner.Focus();
}

bool PropertyEditorImpl::Dialog::OnDragDropReceiveExternal(const Reflex::Object & object)
{
	Field & value = *m_fields[1];

	if (Data::GetBool(value.m_popup, GLX::kWantsFocus))
	{
		typedef ObjectOf < Array <WString> > Files;

		if (auto files = DynamicCast<Files>(object))
		{
			WString path = files->value.GetFirst();

			//value.m_type = Interface::kTypeBinary;

			//value.m_value = File::Open(path);

			value.Update();
		}
	}

	return true;
}

const Key32 PropertyEditorImpl::PropertyEditorImpl::kTypeLabelID = "type";

PropertyEditorImpl::PropertyEditorImpl::ModificationScope::ModificationScope(PropertyEditorImpl & view)
	: view(view),
	iface(view.interface),
	data(view.m_root)
{
	Retain(data);

	g_n++;
}

PropertyEditorImpl::PropertyEditorImpl::ModificationScope::~ModificationScope()
{
	Release(data);

	view.Update();
}

PropertyEditorImpl::PropertyEditorImpl::Transaction::Transaction(PropertyEditorImpl & view)
	: ModificationScope(view)
{
	m_undo.Clear();

	iface.Serialize(data, m_undo);
}

PropertyEditorImpl::PropertyEditorImpl::Transaction::~Transaction()
{
	m_redo.Clear();

	iface.Serialize(data, m_redo);

	Data::Archive archive;

	Data::Serialize(archive, m_undo, m_redo);

	view.Commit(std::move(archive));
}

PropertyEditorImpl::PropertyEditorImpl::Attribute::Attribute(PropertyEditorImpl & view)
	: view(view)
{
	GLX::EnableMouse(*this, false);

	GLX::EnableAutoFit(*this, false, true);

	GLX::EnableAutoFit(value, false, true);

	GLX::AddInline(*this, key);

	GLX::AddInlineFlex(*this, value);
}

void PropertyEditorImpl::PropertyEditorImpl::Attribute::Set(const Interface::PropertyRef & property)
{
	propertyref = property;

	EnableOnClock(false);

	if (auto value = Data::GetKey(view.m_keymap, property.a.id))
	{
		GLX::SetText(this->key, ToWString(value));
	}
	else
	{
		GLX::SetText(this->key, ToWString(Data::BytesToHex(Data::Pack(property.a.id))));
	}

	REFLEX_ASSERT(property.c);

	GLX::SetText(this->value, ToWString(property.c(view.m_keymap, property.b)));
}

void PropertyEditorImpl::PropertyEditorImpl::Attribute::OnSetStyle(const GLX::Style & style)
{
	key.SetStyle(style["key"]);

	value.SetStyle(style["value"]);

	type.SetStyle(style["type"]);
}

struct PropertyEditorImpl::iNull : public PropertyEditor::Interface
{
	Pair < Reflex::Detail::DynamicTypeRef, TRef <Data::PropertySet> > GetObjectType() const override { return { Data::PropertySet::kDynamicTypeInfo, Data::PropertySet::null }; }
};

PropertyEditorImpl::iNull gNullInterface;

REFLEX_END_INTERNAL

Reflex::IDE::Detail::PropertyEditorImpl::Interface & Reflex::IDE::Detail::PropertyEditorImpl::Interface::null = Reflex::IDE::Detail::gNullInterface;

Reflex::TRef <Reflex::IDE::Detail::PropertyEditor> Reflex::IDE::Detail::PropertyEditor::Create(const Data::Detail::StandardPropertySheetInterface & propertysheet_interface, const Interface & iface)
{
	return REFLEX_CREATE(PropertyEditorImpl, propertysheet_interface, iface);
}

Reflex::IDE::Detail::PropertyEditor::PropertyEditor(const Interface & iface)
	: Data::History(kMaxUInt16)
	, Data::iStreamable(1)
	, interface(iface)
	, ide_styles(RetrieveStyleSheet())
{
}
