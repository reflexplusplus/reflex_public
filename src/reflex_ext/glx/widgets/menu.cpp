#include "menu.h"
#include "../../../../include/reflex_ext/glx/widgets/popup.h"




//
//menu

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

TRef <Object> CreateWithLabel(const WString::View & label, const Style & style)
{
	auto item = REFLEX_CREATE(Object);

	SetText(*item, label);

	item->SetStyle(style);

	return item;
}

struct ContextMenu : public MenuImpl
{
	static TRef <Menu> Create(Object & src, Key32 context, const Style & style)
	{
		CloseContextMenu();

		auto self = REFLEX_CREATE(ContextMenu, src, context);

		WarnScope <false> scope(null_object_set);

		Object::null.SetProperty(K32("context_menu"), Cast<Menu>(self));

		self->SetStyle(style);
		
		return self;
	}

	ContextMenu(Object & src, Key32 context)
		: m_source(src),
		m_oninit(Core::desktop->CreateAnimationClock([this, position = GetPointerPosition(src.GetWindow())](Float)
		{
			m_oninit.Clear();

			Detail::PlacePopup(m_source, m_source->GetWindow()->GetForeground(), *this, { position, kNormal }, kAlignmentBottom, kOrientationNear);
		}))
	{
		MenuImpl::m_context = context;

		st_self = this;
	}

	~ContextMenu()
	{
		st_self = nullptr;
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == kRequestClose)
		{
			Detach();

			return true;
		}

		return MenuImpl::OnEvent(src, e);
	}


	const Core::WeakReference m_source;

	Reference <AbstractProperty> m_oninit;

	inline static ContextMenu * st_self = nullptr;
};

GLX::Object * LookupMouseOver(Object & object)
{
	Rect rect = { {}, object.GetRect().size };

	auto pointer_position = TransformPosition(object, GetPointerPosition(object.GetWindow()));

	if (Contains(rect, pointer_position))
	{
		for (auto & i : object) if (Contains(i.GetRect(), pointer_position)) return &i;
	}

	return nullptr;
}

bool IsSeparator(Object * item)
{
	if (item) return Data::GetBool(*item, MenuImpl::kseparator);

	return false;
}

void CloseSubMenu(MenuImpl & self)
{
	if (auto popup = Detail::QueryPopup(self))
	{
		REFLEX_ASSERT(DynamicCast<Menu>(*popup));

		auto submenu = Cast<MenuImpl>(popup);

		GLX::Select(submenu->m_parent_item, false);	//no event here, so safe

		submenu->Detach(); //dont reference self after this
	}
}

void Highlight(Object & object, bool value = true)
{
	SetState(object, kHoverState, value);
}

REFLEX_END_INTERNAL

Reflex::GLX::MenuImpl::MenuImpl()
	: m_cstyle(New<ComputedStyle>())
	, m_root(this)
{
	EnableAbsolute(*this);

	EnableAutoFit(*this, true, true);

	SetFlow(m_content, kFlowY);

	SetContent(m_content);

	EnableOnAttachDetachWindow();
}

void Reflex::GLX::MenuImpl::Clear()
{
	DetachClock(*this, kNullKey);

	m_content.Clear();
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::MenuImpl::AddItem(TRef <Object> item)
{
	item->SetMouseCursor(kMouseCursorPointer);

	SetEventDelegate(item, kNullKey, [item](Object & src, Event & e)
	{
		if (e.id == kMouseDown)
		{
			if (auto menu = Cast<MenuImpl>(QueryParentByType<Menu>(src)))
			{
				if (auto idx = GLX::LookupBranchIndex(menu->GetContent(), item))
				{
					auto proot = Cast<MenuImpl>(menu->m_root);

					Core::WeakReference rootref(*proot);

					GLX::Emit(*menu, kMenuSelect, kindex, idx.value, kitem, item);

					rootref->Detach();
				}
			}
		}

		return false;
	});

	return AddInline(m_content, item);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::MenuImpl::AddSeparator()
{
	auto item = New<Object>();

	item->SetStyle(m_cstyle->kSeparator);

	return AddSeparator(item);
}

Reflex::TRef <Reflex::GLX::Menu> Reflex::GLX::MenuImpl::AddSubMenu(TRef <Object> item, TRef <Menu> base)
{
	auto menu = Cast<MenuImpl>(base);

	menu->m_root = m_root;

	menu->m_parent_item = item;


	item->SetMouseCursor(kMouseCursorPointer);

	item->SetProperty(kNullKey, menu);

	AddInline(m_content, item);


	BindEvent(item, kMouseDown, [item](Object & src, Event & e)
	{
		if (auto menu = QueryParentByType<Menu>(src))
		{
			menu->OpenSubMenu(item);
		}

		return true;
	});

	return menu;
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::MenuImpl::AddItem(const WString::View & label)
{
	return AddItem(CreateWithLabel(label, m_cstyle->kItem));
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::MenuImpl::AddSeparator(TRef <Object> item)
{
	BindEvent(item, kMouseDown, [](Object & src, Event & e){ return true; });

	Data::SetBool(item, kseparator, true);

	return AddInline(m_content, item);
}

Reflex::TRef <Reflex::GLX::Menu> Reflex::GLX::MenuImpl::AddSubMenu(const WString::View & label)
{
	auto submenu = AddSubMenu(CreateWithLabel(label, m_cstyle->kFolder), Create());

	submenu->SetStyle(GetStyle());

	submenu->SetState(K32("sub"));

	return submenu;
}

bool Reflex::GLX::MenuImpl::OpenSubMenu(Object & item)
{
	if (auto window = GetWindow())
	{
		if (auto submenu = item.QueryProperty<MenuImpl>(kNullKey))
		{
			if (!submenu->GetWindow())
			{
				CloseSubMenu(*this);

				ComputeLayout();

				auto target_rect = CalculateAbsoluteRect(item);

				target_rect = Indent(target_rect, item.GetComputedStyle()->GetPadding());

				Select(item, true);

				Detail::PlacePopup(*this, window->GetForeground(), *submenu, target_rect, kAlignmentRight, kOrientationNear);

				return true;
			}
		}
		else
		{
			CloseSubMenu(*this);
		}
	}

	return false;
}

void Reflex::GLX::MenuImpl::OnSetStyle(const GLX::Style & style)
{
	m_cstyle = Detail::Compile<ComputedStyle>(style);

	ScrollArea::OnSetStyle(style);
}

bool Reflex::GLX::MenuImpl::OnEvent(Object & src, Event & e)
{
	constexpr auto RevealAndFocus = [](ScrollArea & menu, Object * item, bool down)
	{
		if (item)
		{
			Object*(Object::*GetNextFn[2])() = {&Object::GetPrev, &Object::GetNext};

			while (IsSeparator(item)) item = (item->*GetNextFn[down])();

			if (item)
			{
				auto & rect = item->GetRect();

				menu.Reveal(true, rect.origin.y, rect.size.h, rect.size.h);

				item->Focus();

				return true;
			}
		}

		return false;
	};

 	if (e.id == kKeyDown)
	{
		auto keycode = GetKeyCode(e);

		switch (keycode)
		{
		case kKeyCodeDown:
			if (auto idx = LookupBranchIndex(m_content, Core::desktop->GetFocus()))
			{
				auto item = LookupChildAtIndex(m_content, idx.value)->GetNext();

				if (RevealAndFocus(*this, item, true)) return true;
			}
			else if (RevealAndFocus(*this, m_content.GetFirst(), true))
			{
				return true;
			}
			break;

		case kKeyCodeUp:
			if (auto idx = LookupBranchIndex(m_content, Core::desktop->GetFocus()))
			{
				auto item = LookupChildAtIndex(m_content, idx.value)->GetPrev();

				if (RevealAndFocus(*this, item, false)) return true;
			}
			else if (RevealAndFocus(*this, m_content.GetLast(), false))
			{
				return true;
			}
			break;

		case kKeyCodeHome:
			RevealAndFocus(*this, m_content.GetFirst(), true);
			return true;

		case kKeyCodeEnd:
			RevealAndFocus(*this, m_content.GetLast(), false);
			return true;

		case kKeyCodeLeft:
			Detach();
			return true;

		case kKeyCodeRight:
		case kKeyCodeEnter:
			if (auto idx = GLX::LookupBranchIndex(m_content, Core::desktop->GetFocus()))
			{
				auto item = GLX::LookupChildAtIndex(m_content, idx.value);

				if (auto menu = item->QueryProperty<MenuImpl>(kNullKey))
				{
					OpenSubMenu(item);

					if (auto first = menu->m_content.GetFirst()) first->Focus();
				}
				else if (keycode == kKeyCodeEnter)
				{
					GLX::Emit(item, kMouseDown);
				}
			}
			return true;

		case kKeyCodeEscape:
			Detach();
			return true;

		default:
			break;	//defer to scroller
			//return true;	//was this intentional?
		};
	}
	else if (e.id == kCharacter)
	{
		auto character = Lowercase(GetKeyCharacter(e));

		UInt n = m_content.GetNumItem();

		auto pfocus = Core::desktop->GetFocus().Adr();

		UInt start = 0;

		if (pfocus->GetParent() == m_content)
		{
			start = GLX::LookupIndex(*pfocus).value;

			start = Modulo(start + 1, n);
		}

		Array <Object*> list;

		auto ptr = Extend(list, n).data;

		for (auto & i : m_content) *ptr++ = &i;

		REFLEX_LOOP(idx, n)
		{
			auto & item = *list[Modulo(idx + start, n)];

			auto label = GetText(item);

			if (Lowercase(label.data[0]) == character)
			{
				if (!BranchContains(item, Core::desktop->GetFocus()))
				{
					RevealAndFocus(*this, &item, true);

					break;
				}
			}
		}

		return true;
	}
	else if (e.id == kDragDropTender)
	{
		if (auto pmenuref = QueryDragDropData< ObjectOf<Menu*> >(e))
		{
			if (pmenuref->value == this)
			{
				for (auto & i : BranchIterator(m_content)) Highlight(i, false);

				if (auto mouseover = LookupMouseOver(m_content))
				{
					Highlight(*mouseover);

					return true;
				}
			}
		}
	}
	else if (e.id == kDragDropReceive)
	{
		if (auto mouseover = LookupMouseOver(m_content))
		{
			GLX::Emit(*mouseover, kMouseDown);
		}

		return true;
	}

	return ScrollArea::OnEvent(src, e);
}

void Reflex::GLX::MenuImpl::OnAttachWindow()
{
	Object * src = this;

	if (auto owner = QueryProperty<Reflex::Detail::WeakRef<Object>>(kowner)) src = owner->Load().Adr();

	EmitMenuOpenEvent(*src, *this, m_context);


	REFLEX_ASSERT(Core::desktop->GetFocus().Adr() != this);

	
	//mouseover delegate

	SetAbstractProperty(*this, kHoverState, Core::desktop->CreateListener(Core::Desktop::kNotificationMouseOver, [this]()
	{
		auto & content = m_content;

		if (auto idx = LookupBranchIndex(content, Core::desktop->GetMouseOver()))
		{
			AttachPeriodicClock(*m_root, kNullKey, 0.35f, [this, ref = Core::WeakReference(LookupChildAtIndex(content, idx.value))]()
			{
				auto & item = *ref;

				if (BranchContains(item, Core::desktop->GetMouseOver())) OpenSubMenu(item);

				DetachClock(*m_root, kNullKey);
			});
		}
	}));


	//support popup handler

	GLX::ScrollArea::OnAttachWindow();


	//display

	while (IsSeparator(m_content.GetLast()))
	{
		m_content.GetLast()->Detach();
	}

	if (GetContent()->Empty())
	{
		SetStyle(Style::null);

		AttachAnimationClock(*this, kNullKey, [this](Float32 a)
		{
			Detach();
		});
	}
}

void Reflex::GLX::MenuImpl::OnDetachWindow()
{
	DetachClock(*this, kNullKey);	//this not root

	UnsetProperty<Reflex::Object>(kFocusedState);

	UnsetProperty<Reflex::Object>(kHoverState);

	CloseSubMenu(*this);

	GLX::ScrollArea::OnDetachWindow();
}

Reflex::GLX::MenuImpl::ComputedStyle::ComputedStyle()
{
}

Reflex::GLX::MenuImpl::ComputedStyle::ComputedStyle(const Style & style)
	: kFolder(style[K32("folder")])
	, kItem(style[K32("item")])
	, kSeparator(style[kseparator])
{
}

Reflex::TRef <Reflex::GLX::Menu> Reflex::GLX::Menu::Create()
{
	return REFLEX_CREATE(MenuImpl);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::AddMenuSection(Menu & menu, const WString::View & label)
{
	auto item = CreateWithLabel(label, menu.GetStyle()["section"]);

	return menu.AddSeparator(item);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::AddMenuOption(Menu & menu, const WString::View & label, bool selected)
{
	auto item = menu.AddItem(CreateWithLabel(label, menu.GetStyle()["option"]));

	Select(item, selected);

	return item;
}

void Reflex::GLX::BindMenuSelect(Menu & menu, const Function <void(UInt)> & onselect)
{
	BindEvent(menu, Menu::kMenuSelect, [onselect](Object &, Event & e)
	{
		onselect(GetIndex(e));

		return true;
	});
}

Reflex::Reference <Reflex::GLX::Menu> Reflex::GLX::OpenContextMenu(Object & src, Key32 context, Key32 style)
{
	return ContextMenu::Create(src, context, FindStyle(src, style));
}

void Reflex::GLX::CloseContextMenu()
{
	if (ContextMenu::st_self) ContextMenu::st_self->Detach();
}

Reflex::TRef <Reflex::GLX::Menu> Reflex::GLX::Detail::GetMenuProperty(Event & e)
{
	return e.QueryProperty<Menu>(kmenu, &Reflex::Detail::GetNullInstance<Menu>());
}

REFLEX_NOINLINE bool Reflex::GLX::Detail::EmitMenuOpenEvent(Object & src, Menu & menu, Key32 context)
{
	AnimationScope scope(false);

	auto e = Make<Event>(Menu::kMenuOpen);

	e->SetProperty(kmenu, menu);
	e->SetProperty(kmenu, Cast<Object>(menu));	//legacy for VM

	Data::SetKey32(e, kcontext, context);

	Data::SetUInt8(e, kmodifiers, System::GetModifierKeys());

	return src.Emit(e);
}

bool Reflex::GLX::Detail::PopupHasFocus(Object & menu, Object & focus)
{
	if (BranchContains(menu, focus)) return true;

	if (auto sub_popup = QueryPopup(menu))
	{
		return PopupHasFocus(*sub_popup, focus);
	}

	return false;
}
