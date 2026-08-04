#include "console.h"
#include "reflex/glx/detail/event_log.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

extern Tuple < CString::View, UInt32, bool, UInt8 > kRenderModes[GLX::Detail::ComputedStyle::kNumRender];

REFLEX_END

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

class UI : public Detail::ViewInspectorPanel
{
public:

	struct Panel;

	struct Objects;

	struct Events;

	struct Layers;

	struct Performance;

	struct Animations;


	UI();


	void Config(UInt8 flags);

	virtual void SetRoot(GLX::Object & object) override;


	virtual void OnRestore(Data::Archive::View & stream, Key32 context) override;

	virtual void OnStore(Data::Archive & stream) const override;


	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnClock(Float) override;

	virtual void OnUpdate() override;

	virtual void OnAttachWindow() override;

	virtual void OnDetachWindow() override;

	virtual void OnSetProperty(Address address, Reflex::Object & object) override;


	void OnFocus(GLX::Object & object);

	void SetTarget(GLX::Object & object);

	virtual void OnReleaseData() override
	{
	}



	Reference <Reflex::Object> m_onfocus;

	GLX::Core::Desktop::Monitor m_monitor;


	GLX::Core::WeakReference m_root, m_focus;


	UInt8 m_flags = 0;


	Reference <Objects> m_objectsview;

	GLX::Selector m_selector;

	GLX::Object m_header, m_maintabs, m_clientarea;

	GLX::Object m_footer;

	GLX::Button m_show_btn;

	GLX::Button m_track_btn;


	GLX::Object m_focus_displays[2];


	UIntNative m_hash_z;

	GLX::Rect m_focus_rect_z;

};

struct UI::Panel : public GLX::Object
{
	Panel()
	{
		GLX::SetFlow(*this, GLX::kFlowY);

		EnableOnAttachDetachWindow();
	}

	virtual void OnSetRoot(GLX::Object & root) = 0;

	virtual void OnSetTarget(GLX::Object & focus) = 0;

	virtual void OnSetStyle(const ComputedStyle & cstyle) = 0;

	virtual void OnAttachWindow() override { Update(); }
};

struct UI::Objects : public Panel
{
	static constexpr CString::View kKeys[] =
	{
		"Positioning",
		"Flow",
		"AutoFit",
		"Responsive",
		"Position",
		"Abs Position",
		"Size",
		"ContentSize",
		"Scale",

		"Stylesheet",
		"ID",
		"Margin",
		"Padding",
		"Opacity",
		"Clip",
		"Render",
		"Renderer",

		"data",
		"transition",
		"value",
		"range",
		"region",
		"origin",
		"content",
		"{delegates}",
		"{mods}",
		"{states}",
		"{TransactionState}",
		"{Incrementer}"
	};

	static constexpr CString::View kPathDelim = " > ";

	struct Interface : public Detail::PropertyEditor::Interface
	{
		REFLEX_DECLARE_KEY32(Positioning);
		REFLEX_DECLARE_KEY32(Flow);

		Interface(Objects & view);

		Pair < Reflex::Detail::DynamicTypeRef, TRef <Data::PropertySet> > GetObjectType() const override { return { GLX::Object::kDynamicTypeInfo, GLX::Object::null }; }

		virtual ConstTRef <Data::KeyMap> GetKeyMap() const override
		{
			auto keymap = New<Data::KeyMap>();

			for (auto & i : kKeys) Data::RegisterKey(keymap, i);

			Data::Assimilate(keymap, GLX::GetKeyMap());

			return keymap;
		}

		virtual Array <WString> GetPropertyGroups() const override { return { L"Layout", L"Style", L"Properties", L"Delegates"}; }

		virtual void GetChildren(Data::PropertySet & node, Children & children) const override;

		virtual void GetProperties(Data::PropertySet & node, Key32 group, Properties & properties) const override;


		Objects & view;

		mutable Array <Key32> m_ids;

		Map <TypeID,Data::Detail::ObjectToStringFn> m_stringfns;
	};


	Objects(UI & view);


	virtual void OnSetRoot(GLX::Object & root) override;

	virtual void OnSetTarget(GLX::Object & focus) override;


	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnUpdate() override;

	virtual void OnSetStyle(const ComputedStyle & cstyle) override;



	UI & view;


	GLX::Object m_header;

	GLX::Object m_popup;


	Interface m_interface;

	Detail::PropertyEditor & propertyeditor;


	UInt m_mute;


	static const CString::View kFloats[16];

	static const CString::View kInlines[16];

	static const CString::View kFlows[8];
};

struct UI::Events : public Panel
{
	Events(UI & view)
		: m_target_type(GLX::kMouseDown)
		, m_current_uid(0)
	{
		GLX::AddInlineFlex(m_header, m_popup);

		GLX::AddInline(*this, m_header);

		GLX::AddInlineFlex(*this, m_list);

		EnableOnClock();
	}

	void OnSetRoot(GLX::Object & root) override {}

	void OnSetTarget(GLX::Object & focus) override
	{
		//m_target = &focus;
	}

	void OnSetStyle(const ComputedStyle & cstyle) override
	{
		m_header.SetStyle(cstyle.bar);

		m_popup.SetStyle(cstyle.popup);

		m_item_style = cstyle.list_item;

		m_list.SetStyle(cstyle.list);
	}

	bool OnEvent(GLX::Object & object, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e))
		{
			Sequence <WString, Key32> sorted;

			for (auto [id, value] : m_known_ids)
			{
				if (auto string = GLX::GetKey(id))
				{
					sorted.Insert(ToWString(string), id);
				}
				else
				{
					sorted.Insert(ToWString(Data::BytesToHex(Data::Pack(id.value))), id);
				}
			}

			for (auto & [label, id] : sorted)
			{
				menu->AddItem(label)->id = id;
			}

			GLX::BindEvent(menu, GLX::Menu::kMenuSelect, [this](GLX::Object & src, GLX::Event & e)
			{
				m_target_type = GLX::GetItem(e)->id;

				Update();

				return true;
			});
		}
		else if (e.id == GLX::List::kListSelect)
		{
			if (Data::GetBool(e, GLX::kstate))
			{
				UInt idx = GLX::GetIndex(e);
				
				auto & steps = m_current->steps;

				if (idx < steps.GetSize())
				{
					auto receiver = steps[idx].ref.Load();

					receiver->Focus();
				}
			}

			return true;
		}

		return Panel::OnEvent(object, e);
	}

	void OnClock(Float) override
	{
		//m_types.Clear();

		bool update = false;

		GLX::Detail::FlushEventLog([this, &update](GLX::Detail::RecordedEvent & e)
		{
			m_known_ids.Set(e.event_id);

			if (e.event_id == m_target_type)
			{
				if (SetFiltered(m_current_uid, e.uid))
				{
					m_current = e;

					update = true;
				}
			}
		});

		GLX::SetText(m_popup, ToWString(GLX::GetKey(m_target_type)));

		if (update)
		{
			auto content = m_list.GetContent();

			content->Clear();

			UInt idx = m_current->steps.GetSize();

			for (auto & step : ReverseIterate(m_current->steps))
			{
				auto receiver = step.ref.Load();

				auto item = Detail::CreateInfoItem(Join(ToWString(--idx), L':', L' ', ToWString(Data::BytesToHex(Data::Pack(receiver.Adr())))), ToWString(ToView(receiver->object_t->tname)), false);

				item->SetStyle(m_item_style);

				GLX::AddInline(content, item);
			}
		}
	}

	ConstTRef <GLX::Style> m_item_style;

	Map <Key32> m_known_ids;

	Key32 m_target_type;

	UInt32 m_current_uid;

	Reference <GLX::Detail::RecordedEvent,false> m_current;


	GLX::Popup m_popup;
	
	GLX::Object m_header;

	GLX::ListScroller m_list;
};

struct UI::Layers : public Panel
{
	Layers(UI & view);

	virtual void OnSetRoot(GLX::Object & root) override {}

	virtual void OnSetTarget(GLX::Object & focus) override {}


	virtual void OnSetStyle(const ComputedStyle & cstyle) override;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnUpdate() override;


	UI & view;

	GLX::ListScroller m_list;
};

struct UI::Performance : public Panel
{
	Performance(UI & view);


	virtual void OnSetRoot(GLX::Object & root) override;

	virtual void OnSetTarget(GLX::Object & focus) override;


	virtual void OnSetStyle(const ComputedStyle & cstyle) override;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnClock(Float) override;

	virtual void OnUpdate() override;



	UI & view;

	Reflex::Detail::WeakRef <GLX::WindowClient> m_window;


	InfoItem m_object_count, m_style_count, m_font_count, m_layer_count, m_animation_count, m_active_animation_count, m_renderer_count, m_texture_renderer_count, m_state_renderer_count, m_count, m_update_count, m_rebuild_count, m_accommodate_count, m_align_count, m_responsive_align_count;


	Tuple <InfoItem*,const UInt*> m_globalinfos[8];

	Tuple <InfoItem*,UInt16> m_window_infos[GLX::WindowClient::kNumDebugCounter];
};

struct UI::Animations : public Panel
{
	Animations(UI & view);

	virtual void OnSetStyle(const ComputedStyle & cstyle) override;

	virtual void OnClock(Float) override;


	UI & view;

	GLX::ListScroller m_list;

	const GLX::Style * m_item_style;
};

void UI::Config(UInt8 flags)
{
	if (flags & 15)
	{
		auto cstyle = GLX::Detail::Compile<ComputedStyle>(GetStyle());

		auto button = cstyle->button;

		m_selector.Clear();

		m_maintabs.Clear();

		const Pair <WString::View, FunctionPointer<GLX::Object*(UI &)>> panels[] =
		{ 
			{ 
				L"Objects", 
				[](UI & self) -> GLX::Object *
				{
					return self.m_objectsview.Adr();
				} 
			},
			{ 
				L"Events", 
				[](UI & self) -> GLX::Object *
				{
					return New<Events>(self).Adr();
				} 
			},
			{ 
				L"Performance", 
				[](UI & self) -> GLX::Object *
				{
					return New<Performance>(self).Adr();
				} 
			},
			{ 
				L"Layers", 
				[](UI & self) -> GLX::Object *
				{
					return New<Layers>(self).Adr();
				} 
			},
		};

		REFLEX_LOOP(idx, GetArraySize(panels))
		{
			if (BitCheck(flags, idx))
			{
				auto & i = panels[idx];

				auto tab = GLX::Init(REFLEX_CREATE(GLX::Button, i.a), button);

				GLX::AddInline(m_maintabs, tab);

				m_selector.AddPanel(*i.b(*this));
			}
		}

		if (m_maintabs.GetNumItem() == 1) m_header.Detach();

		m_selector.SelectPanel(0);
	}
}

UI::UI()
	: Detail::ViewInspectorPanel("UI", 1)
	, m_monitor(GLX::Core::desktop)
	, m_objectsview(REFLEX_CREATE(Objects, *this))
	, m_show_btn(L"Show")
	, m_track_btn(L"Track")
{
	auto styles = Detail::RetrieveStyleSheet();


	for (auto & i : m_focus_displays) GLX::EnableMouse(i, false);


	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::SetFlow(m_header, GLX::kFlowY);

	GLX::AddInline(m_header, m_maintabs);

	GLX::AddInline(m_footer, m_track_btn);

	GLX::AddInline(m_footer, m_show_btn);


	GLX::AddInline(*this, m_header);

	GLX::AddInlineFlex(*this, m_selector);

	GLX::AddInline(*this, m_footer);


	Config(kMaxUInt8);

	SetStyle(styles);


	EnableOnClock();

	EnableOnAttachDetachWindow();
}

void UI::OnStore(Data::Archive & stream) const
{
	auto index = UInt16(m_selector.GetCurrentIndex().value);

	Data::Serialize(stream, index, m_flags);

	Data::PropertySet propertyset;

	Detail::StoreStreamable(propertyset, m_objectsview->propertyeditor);

	Data::kBinaryFormat->Serialize(stream, propertyset);
}

void UI::OnRestore(Data::Archive::View & stream, Key32 context)
{
	m_selector.SelectPanel(Data::Deserialize<UInt16>(stream));

	Data::Deserialize(stream, m_flags);

	Data::PropertySet propertyset;

	Data::kBinaryFormat->Deserialize(stream, propertyset);

	auto & editor = m_objectsview->propertyeditor;

	Detail::RestoreStreamable(propertyset, context, editor);
}

void UI::OnSetStyle(const GLX::Style & style)
{
	auto cstyle = GLX::Detail::Compile<ComputedStyle>(style);

	auto bar = cstyle->bar;

	auto button = cstyle->button;

	m_header.SetStyle(bar);

	m_footer.SetStyle(bar);

	for (auto & i : m_maintabs) i.SetStyle(button);

	m_show_btn.SetStyle(button);

	m_track_btn.SetStyle(button);

	m_focus_displays[0].SetStyle(cstyle->focus_rectangle);

	m_focus_displays[1].SetStyle(cstyle->focus_parent_rectangle);

	REFLEX_LOOP(idx, m_selector.GetNumPanel())
	{
		Cast<Panel>(m_selector.GetPanel(idx))->OnSetStyle(cstyle);
	}
}

bool UI::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		if (!(GLX::GetClickFlags(e) & GLX::kClickFlagRmb))
		{
			auto parent = src.GetParent();

			if (parent == m_maintabs)
			{
				m_selector.SelectPanel(GLX::LookupIndex(src).value);

				return true;
			}
			else if (parent == m_footer)
			{
				m_flags = BitFlip(m_flags, GLX::LookupIndex(src).value);

				Update();
			}
		}
	}
	else if (e.id == GLX::kKeyDown)
	{
		auto keycode = GLX::GetKeyCode(e);

		if (keycode == GLX::kKeyCodeF5)
		{
			GLX::Restart();

			return true;
		}
	}
	else if (e.id == GLX::Selector::kSelectPanel)
	{
		Update();

		return true;
	}

	return ConsolePanel::OnEvent(src, e);
}

void UI::SetRoot(GLX::Object & object)
{
	m_root = object;

	REFLEX_LOOP(idx, m_selector.GetNumPanel())
	{
		auto panel = Cast<Panel>(m_selector.GetPanel(idx));

		panel->OnSetRoot(object);

		panel->Update();
	}

	SetTarget(object);
}

void UI::SetTarget(GLX::Object & object)
{
	m_focus = object;

	REFLEX_LOOP(idx, m_selector.GetNumPanel())
	{
		auto panel = Cast<Panel>(m_selector.GetPanel(idx));

		panel->OnSetTarget(object);

		panel->Update();
	}
}

void UI::OnFocus(GLX::Object & object)
{
	//if (object.GetWindow() != GetWindow() && !m_embed)
	//{
	//	SetRoot(object.GetWindow()->GetContent());
	//}

	if (GLX::BranchContains(m_root, object))
	{
		if (m_focus.Adr() != &object)
		{
			m_focus = object;

			SetTarget(object);

			m_focus_rect_z = {};
		}

		return;
	}
}

void UI::OnClock(Float)
{
	if (m_monitor.Poll())
	{
		auto & root = *m_root;

		UInt count = 0;

		UIntNative hash = 0;

		for (auto & i : Object::BranchIterator(root))
		{
			hash ^= ToUIntNative(&i);

			++count;
		}

		if (SetFiltered(m_hash_z, hash))
		{
			if (m_onfocus) OnFocus(GLX::Core::desktop->GetFocus());

			m_selector.Update();
		}
	}

	if (SetFiltered(m_focus_rect_z, m_focus->GetRect()))
	{
		TRef <GLX::Object> focus = m_focus;

		for (auto & i : m_focus_displays)
		{
			auto rect = GLX::CalculateAbsoluteRect(focus);

			i.SetPosition(rect.origin);

			SetBounds(i, kNullKey, rect.size);

			focus = focus->GetParent();
		}

		auto fg = m_root->GetWindow()->GetForeground();

		if (m_focus_displays[0].GetParent() != fg)
		{
			AddAbsolute(fg, m_focus_displays[1]);

			AddAbsolute(fg, m_focus_displays[0]);
		}

		m_monitor.Poll();	//because just changed the desktop
	}
}

void UI::OnAttachWindow()
{
	if (!m_root)
	{
		GLX::Core::desktop->EnumerateWindows([this](GLX::WindowClient & window)
		{
			if (!DynamicCast<Console::WindowDelegate>(window))
			{
				SetRoot(window.GetContent());
			}
		});
	}

	Update();
}

void UI::OnDetachWindow()
{
	for (auto & i : m_focus_displays) i.Detach();

	m_onfocus.Clear();
}

void UI::OnUpdate()
{
	GLX::SelectBranch(m_maintabs, false);

	GLX::Select(GLX::LookupChildAtIndex(m_maintabs, m_selector.GetCurrentIndex().value));

	if (BitCheck(m_flags, 0))
	{
		m_onfocus = GLX::Core::desktop->CreateListener(GLX::Core::Desktop::kNotificationFocus, [this]()
		{
			OnFocus(GLX::Core::desktop->GetFocus());
		});

		m_track_btn.SetState(GLX::kSelectedState);
	}
	else
	{
		m_onfocus.Clear();

		m_track_btn.UnsetState(GLX::kSelectedState);
	}

	auto show = BitCheck(m_flags, 1);

	GLX::Select(m_show_btn, show);

	for (auto & i : m_focus_displays)
	{
		GLX::EnableDraw(i, kNullKey, show);
	}
}

void UI::OnSetProperty(Address address, Reflex::Object & object)
{
	if (address == MakeAddress<GLX::Object>(K32("Root")))
	{
		AutoRelease(object);

		SetRoot(Cast<GLX::Object>(object));
	}
	else
	{
		GLX::Object::OnSetProperty(address, object);
	}
}

const CString::View kComma = ", ";

const CString::View UI::Objects::kFloats[16] =
{
	"Float: TopLeft",
	"Float: Top",
	"Float: TopRight",
	"Float: Fit, Top",

	"Float: Left",
	"Float: Center",
	"Float: Right",
	"Float: Fit, Center",

	"Float: BottomLeft",
	"Float: Bottom",
	"Float: BottomRight",
	"Float: Fit, Bottom",

	"Float: Left, Fit",
	"Float: Center, Fit",
	"Float: Right, Fit",
	"Float: Fit, Fit",
};

const CString::View UI::Objects::kInlines[16] =
{
	"Inline: Near",
	"Inline: Near",
	"Inline: Near",
	"InlineFlex: Near",

	"Inline: Center",
	"Inline: Center",
	"Inline: Center",
	"InlineFlex: Center",

	"Inline: Far",
	"Inline: Far",
	"Inline: Far",
	"InlineFlex: Far",

	"Inline: Fit",
	"Inline: Fit",
	"Inline: Fit",
	"InlineFlex: Fit",
};

const CString::View UI::Objects::kFlows[8] =
{
	"X",
	"Y",
	"X | Invert",
	"Y | Invert",

	"X | Center",
	"Y | Center",
	"X | Invert | Center",
	"Y | Invert | Center",
};

UI::Objects::Objects(UI & view)
	: view(view)
	, m_interface(*this)
	, propertyeditor(Detail::PropertyEditor::Create(Data::Detail::g_standard_propertysheet_interface, m_interface))
	, m_mute(0)
{
	propertyeditor.id = K32("PropertyEditor");

	GLX::SetFlow(m_header, GLX::kFlowY);

	Data::SetKey32(m_popup, K32("overflow"), kOverflowPath);

	GLX::AddInline(*this, m_header);

	GLX::AddInlineFlex(*this, propertyeditor);
}

void UI::Objects::OnSetRoot(GLX::Object & root)
{
	m_mute++;

	propertyeditor.SetRoot(root);

	m_mute--;
}

void UI::Objects::OnSetTarget(GLX::Object & focus)
{
	m_mute++;

	propertyeditor.SetFocus(focus);

	m_mute--;
}

void UI::Objects::OnSetStyle(const ComputedStyle & cstyle)
{
	m_popup.SetStyle(cstyle.popup);
}

bool UI::Objects::OnEvent(Object & src, GLX::Event & e)
{
	if (e.id == Detail::PropertyEditor::kSelectNode)
	{
		if (!m_mute)
		{
			auto object = Cast<Object>(Data::AcquirePropertySet(e, Detail::PropertyEditor::knode));

			view.OnFocus(object);
		}

		return true;
	}

	return GLX::Object::OnEvent(src, e);
}

void UI::Objects::OnUpdate()
{
	propertyeditor.Update();
}

void UI::Objects::Interface::GetChildren(Data::PropertySet & node, Children & children) const
{
	auto object = Cast<GLX::Object>(node);

	for (auto & i : object)
	{
		if (auto text = GLX::GetText(i))
		{
			children.Push({ Data::Detail::ReadLine(text), i });
		}
		else
		{
			children.Push({ {}, i });
		}
	}
}

UI::Objects::Interface::Interface(Objects & view)
	: view(view)
{
	typedef ObjectOf <GLX::Margin> MarginObject;

	constexpr auto pairfloat = [](const Data::KeyMap &, const Object & object)
	{
		auto [w, h] = Cast<GLX::SizeProperty>(object)->value;

		return Join(ToCString(w, 2, true), ',', ' ', ToCString(h, 2, true));
	};

	m_stringfns[REFLEX_TYPEID(Data::CStringProperty)] = [](const Data::KeyMap &, const Object & object)
	{
		return Cast<Data::CStringProperty>(object)->value;
	};

	m_stringfns[REFLEX_TYPEID(Data::BoolProperty)] = &Data::Detail::BoolToString;

	m_stringfns[REFLEX_TYPEID(ObjectOf<Pair<bool>>)] = [](const Data::KeyMap &, const Object & object)
	{
		auto [a,b] = Cast<ObjectOf<Pair<bool>>>(object)->value;
		
		auto false_true = Reflex::Detail::kFalseTrue;

		return Join(false_true[a], ',', ' ', false_true[b]);
	};

	m_stringfns[REFLEX_TYPEID(Data::Float32Property)] = &Data::Detail::Float32ToString;

	m_stringfns[REFLEX_TYPEID(Data::Int32Property)] = &Data::Detail::Int32ToString;

	m_stringfns[REFLEX_TYPEID(GLX::SizeProperty)] = pairfloat;

	m_stringfns[REFLEX_TYPEID(GLX::PointProperty)] = pairfloat;

	m_stringfns[REFLEX_TYPEID(GLX::RangeProperty)] = pairfloat;

	m_stringfns[REFLEX_TYPEID(MarginObject)] = [](const Data::KeyMap &, const Object & object)
	{
		auto & margin = Cast<MarginObject>(object)->value;

		return Join(Data::Detail::Float64ToString(margin.near.w, 1), ',', ' ', Data::Detail::Float64ToString(margin.near.h, 1), ',', ' ', Data::Detail::Float64ToString(margin.far.w, 1), ',', ' ', Data::Detail::Float64ToString(margin.far.h, 1));
	};
}

void UI::Objects::Interface::GetProperties(Data::PropertySet & node, Key32 group, Properties & properties) const
{
	constexpr auto MakeStringProperty = [](UInt32 id, const CString::View & string) -> PropertyRef
	{
		return { MakeAddress<Data::CStringProperty>(id), New<Data::CStringProperty>(string) };
	};

	constexpr auto MakeBoolProperty = [](UInt32 id, bool value) -> PropertyRef
	{
		return { MakeAddress<Data::BoolProperty>(id), New<Data::BoolProperty>(value) };
	};

	constexpr auto MakePairOfBoolProperty = [](UInt32 id, Pair <bool> values) -> PropertyRef
	{
		return { MakeAddress<ObjectOf<Pair<bool>>>(id), New<ObjectOf<Pair<bool>>>(values) };
	};

	constexpr auto MakeFloatProperty = [](UInt32 id, Float32 value) -> PropertyRef
	{
		return { MakeAddress<Data::Float32Property>(id), New<Data::Float32Property>(value) };
	};
	
	constexpr auto MakeSizeProperty = [](UInt32 id, GLX::Size value) -> PropertyRef
	{
		return { MakeAddress<GLX::SizeProperty>(id), New<GLX::SizeProperty>(value) };
	};

	constexpr auto MakeMarginProperty = [](UInt32 id, const GLX::Margin & value) -> PropertyRef
	{
		return { MakeAddress<GLX::MarginProperty>(id), New<GLX::MarginProperty>(value) };
	};

	auto object = Cast<GLX::Object>(node);

	switch(group.value)
	{
	case K32("Layout"):
	{
		auto positioning = GLX::Detail::GetPositioning(object);

		auto layout = object->GetLayoutFlags();

		UInt pos = (positioning.b + (positioning.c * 4)) & 15;

		switch(positioning.a)
		{
		case GLX::Detail::kPositioningInline:
			properties.Push(MakeStringProperty(kPositioning, kInlines[pos]));
			break;

		case GLX::Detail::kPositioningFloat:
			properties.Push(MakeStringProperty(kPositioning, kFloats[pos]));
			break;

		default:
			properties.Push(MakeStringProperty(kPositioning, "Absolute"));
			break;
		}

		properties.Push(MakeStringProperty(kFlow, kFlows[layout & 7]));

		properties.Push(MakePairOfBoolProperty(K32("AutoFit"), MakeTuple(BitCheck(layout, GLX::Detail::kStandardLayoutAutofitX), BitCheck(layout, GLX::Detail::kStandardLayoutAutofitY))));

		properties.Push(MakeBoolProperty(K32("Responsive"), object->isresponsive));

		properties.Push(MakeSizeProperty(K32("ContentSize"), object->contentsize));

		auto rect = object->GetRect();

		properties.Push(MakeSizeProperty(K32("Position"), Reinterpret<GLX::Size>(rect.origin)));

		properties.Push(MakeSizeProperty(K32("Size"), rect.size));

		properties.Push(MakeSizeProperty(K32("Scale"), Reinterpret<GLX::Size>(object->GetScale())));

		properties.Push(MakeSizeProperty(K32("Abs Position"), Reinterpret<GLX::Size>(GLX::CalculateAbs(object).a)));
	}
	break;

	case K32("Style"):
	{
		m_ids.Clear();

		auto style = object->GetCurrentStyle();

		auto & sheet = properties.Push(MakeStringProperty(K32("Stylesheet"), {}));

		auto & id = properties.Push(MakeStringProperty(K32("ID"), {}));

		for (auto & i : GLX::Style::ConstParentRange(*style))
		{
			if (auto proot = DynamicCast<GLX::StyleSheet>(i))
			{
				File::ResourcePool::Lock lock(GLX::Core::desktop->resourcepool);

				Cast<Data::CStringProperty>(sheet.b)->value = ToCString(File::GetPath(lock, MakeAddress<GLX::StyleSheet>(proot->path)));

				auto map = Data::GetKeyMap(*proot);

				if (m_ids)
				{
					CString path;

					for (auto & i : ReverseIterate(m_ids))
					{
						if (auto id = Data::GetKey(map, i))
						{
							path.Append(id);
						}
						else
						{
							path.Append(Data::BytesToHex(Data::Pack(id)));
						}

						path.Append(kPathDelim);
					}

					path.Shrink(3);

					Cast<Data::CStringProperty>(id.b)->value = path;
				}

				break;
			}
			else
			{
				m_ids.Push(i.id);
			}
		}

		auto cstyle = GLX::Detail::Compile<GLX::Detail::ComputedStyle>(style);

		properties.Push(MakeMarginProperty(K32("Margin"), cstyle->GetMargin()));

		properties.Push(MakeMarginProperty(K32("Padding"), cstyle->GetPadding()));

		properties.Push(MakeFloatProperty(K32("Scale"), cstyle->GetScale()));

		properties.Push(MakeFloatProperty(K32("Opacity"), cstyle->GetOpacity()));

		properties.Push(MakePairOfBoolProperty(K32("Clip"), cstyle->GetClip()));

		properties.Push(MakeStringProperty(K32("Render"), GLX::Detail::kRenderModes[cstyle->GetRender()].a));

		properties.Push(MakeStringProperty(K32("Renderer"), object->GetRenderer()->object_t->tname));
	}
	break;

	case K32("Delegates"):
	{
		UInt idx = 0;

		object->EnumerateDelegates([&properties, &idx](Object & delegate)
		{
			Address address = { idx++, delegate.object_t->type_id };

			properties.Push({ address, delegate });
		});
	}
	break;

	default:
	{
		for (auto & i : node.Iterate())
		{
			properties.Push({ i.key, i.value });
		}
	}
	break;
	}

	Data::Detail::ObjectToStringFn null = [](const Data::KeyMap&,const Object & object) -> CString
	{
		return object.object_t->tname;
	};
	
	for (auto & i : properties)
	{
		i.c = *m_stringfns.Search(i.a.type_id, &null);
	}
}

UI::Layers::Layers(UI & ui)
	: view(ui)
{
	m_list.GetContent()->SetSelectionMode(GLX::AbstractList::kSelectionModeMultiToggle);

	GLX::AddStretch(*this, m_list);
}

void UI::Layers::OnSetStyle(const ComputedStyle & cstyle)
{
	m_list.SetStyle(cstyle.list);
}

bool UI::Layers::OnEvent(Object & src, GLX::Event & e)
{
	if (e.id == GLX::AbstractList::kListSelect)
	{
		auto item = GLX::LookupChildAtIndex(m_list.GetContent(), GLX::GetIndex(e));

		if (auto cls = ToPointer<GLX::Detail::Layer::Class>(UIntNative(Data::GetUInt64(item, K32("ID64")))))
		{
			cls->enabled = Data::GetBool(e, GLX::kstate);

			GLX::Restart();

			return true;
		}
	}

	return GLX::Object::OnEvent(src, e);
}

void UI::Layers::OnUpdate()
{
	auto cstyle = GLX::Detail::Compile<ComputedStyle>(view.GetStyle());

	auto content = m_list.GetContent();

	content->Clear();

	for (auto & i : GLX::Detail::Layer::Class::range)
	{
		auto item = GLX::AddInline(content, Detail::CreateInfoItem(ToWString(i.id), {}, false));

		Data::SetUInt64(item, K32("ID64"), ToUIntNative(&i));

		GLX::Select(item, i.enabled);

		item->SetStyle(cstyle->list_item);
	}
}

UI::Performance::Performance(UI & view)
	: view(view),
	m_object_count(L"Objects"),
	m_style_count(L"Styles"),
	m_font_count(L"Fonts"),
	m_layer_count(L"Layers"),
	m_animation_count(L"Animations"),
	m_active_animation_count(L"Active Animations"),
	m_renderer_count(L"Renderers"),
	m_texture_renderer_count(L"Texture Renderers"),
	m_state_renderer_count(L"Renderers with state"),

	m_count(L"Object Count"),
	m_update_count(L"Update Count"),
	m_rebuild_count(L"Rebuild Count"),
	m_accommodate_count(L"Accommodate Count"),
	m_align_count(L"Align Count"),
	m_responsive_align_count(L"Responsive Align Count")
{
	m_globalinfos[0] = { &m_object_count, &GLX::Object::GetCount() };
	m_globalinfos[1] = { &m_style_count, &GLX::Style::GetCount() };
	m_globalinfos[2] = { &m_layer_count, &GLX::Detail::Layer::GetCount() };
	m_globalinfos[3] = { &m_font_count, &GLX::Detail::Font::GetCount() };
	m_globalinfos[4] = { &m_animation_count, &GLX::Animation::GetCount() };
	m_globalinfos[5] = { &m_active_animation_count, &GLX::Detail::GetNumActiveAnimation() };
	m_globalinfos[6] = { &m_renderer_count, &GLX::Detail::Countable<K32("Renderer")>::GetCount()};
	m_globalinfos[7] = { &m_texture_renderer_count, &GLX::Detail::Countable<K32("CachedRenderer")>::GetCount() };

	m_window_infos[GLX::WindowClient::kDebugCounterUpdate] = { &m_update_count, 0 };
	m_window_infos[GLX::WindowClient::kDebugCounterRebuild] = { &m_rebuild_count, 0 };
	m_window_infos[GLX::WindowClient::kDebugCounterAccommodate] = { &m_accommodate_count, 0 };
	m_window_infos[GLX::WindowClient::kDebugCounterAlign] = { &m_align_count, 0 };
	m_window_infos[GLX::WindowClient::kDebugCounterAlignResponsive] = { &m_responsive_align_count, 0 };

	for (auto & i : m_globalinfos) GLX::EnableMouse(GLX::AddInline(*this, i.a), false);

	for (auto & i : m_window_infos) GLX::EnableMouse(GLX::AddInline(*this, i.a), false);

	EnableOnClock();
}

void UI::Performance::OnSetRoot(GLX::Object & root)
{
	m_window.Store(root.GetWindow());
}

void UI::Performance::OnSetTarget(GLX::Object & root)
{
	m_window.Store(root.GetWindow());
}

void UI::Performance::OnSetStyle(const ComputedStyle & cstyle)
{
	m_count.SetStyle(cstyle.list_item);

	for (auto & i : m_globalinfos) i.a->SetStyle(cstyle.list_item);

	for (auto & i : m_window_infos) i.a->SetStyle(cstyle.list_item);
}

bool UI::Performance::OnEvent(Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		for (auto & i : m_window_infos) i.b = 0;

		return true;
	}

	return GLX::Object::OnEvent(src, e);
}

void UI::Performance::OnClock(Float)
{
	WString::View stroke(L" / ");

	for (auto & i : m_globalinfos)
	{
		GLX::SetText(i.a->value, ToWString(*i.b));
	}

	auto pcounters = m_window.Load()->GetDebugCounters();

	for (auto & i : m_window_infos)
	{
		auto count = *pcounters++;

		i.b = Max(i.b, count);

		GLX::SetText(i.a->value, Join(ToWString(count), stroke, ToWString(i.b)));
	}
}

void UI::Performance::OnUpdate()
{
	auto root = m_window.Load()->GLX::Core::WindowClient::GetContent();

	UInt count = 0;

	auto range = Object::BranchIterator(root);

	auto itr = range.begin();

	auto end = range.end();

	for (; itr != end; ++itr) ++count;

	GLX::SetText(m_count.value, ToWString(count));
}

UI::Animations::Animations(UI & ui)
	: view(ui),
	m_item_style(&GLX::Style::null)
{
	m_list.GetContent()->SetSelectionMode(GLX::AbstractList::kSelectionModeMultiToggle);

	GLX::AddStretch(*this, m_list);

	EnableOnClock();
}

void UI::Animations::OnSetStyle(const ComputedStyle & cstyle)
{
	m_list.SetStyle(cstyle.list);

	m_item_style = cstyle.list_item.Adr();
}

void UI::Animations::OnClock(Float)
{
}

Detail::ConsolePanel::Ctr gViewInspectorCtr(L"UI", -2, []() -> TRef <Detail::ConsolePanel>
{
	return REFLEX_CREATE(UI);
});

REFLEX_END_INTERNAL
