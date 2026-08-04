#include "../../../../include/reflex/glx/behaviours/textedit.h"




//
//core

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

#if (defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_IOS))
constexpr bool kApple = true;
#else
constexpr bool kApple = false;
#endif

struct TextEditBehaviourImpl : public TextEditBehaviourWithState
{
	static constexpr WChar kLF = WChar(10);
	static constexpr WChar kCR = WChar(13);
	static constexpr WChar kTabChar = WChar(9);

	static constexpr WString::View kTabSpaces = L"    ";

	REFLEX_DECLARE_KEY32(ClickBlocker);
	REFLEX_DECLARE_KEY32(CaretBlink);
	REFLEX_DECLARE_KEY32(KeepKeyboardAlive);


	struct TransactionScope;

	struct ComputeScope;

	struct VirtualKeyboard;

	typedef const WChar * CaretPtr;



	//lifetime

	using TextEditBehaviourWithState::TextEditBehaviourWithState;



private:

	void RequestTextInput() override;

	void SetInputType(VirtualKeyboardInputType type) override;

	void SetTabSpaces(UInt8 spaces) override { m_tab_spaces = Min<UInt8>(spaces, 4); }

	void SetCaret(UInt pos, UInt selection_start, UInt selection_length) override;

	void Update() override;

	void Reveal() override;

	Float GetLineHeight() const override { return m_lineh; }

	Pair <Float> GetLineCoordinates(UInt idx) const override;

	void OnAttachObject() override;

	void OnDetachObject() override;

	bool OnEvent(GLX::Object & src, Event & e) override;

	void OnRestoreHistory(Data::Archive::View stream, bool redo) override;


	static CaretPtr TransformCaret(const WString::View & actual, const WString::View & transformed, CaretPtr transformed_ptr)
	{
		if (actual.size == transformed.size)
		{
			return actual.data + (transformed_ptr - transformed.data);
		}
		else
		{
			return actual.data;
		}
	}

	static Pair <UInt,WString::View> GetLineEx(WString::View view, CaretPtr caret_ptr)
	{
		UInt idx = 0;

		while (view)
		{
			auto line = Data::Detail::ReadLine(view);

			if (Reflex::Inside(caret_ptr, line.data, line.size + 1)) return { idx, line };

			idx++;
		}

		return { idx, { view.data, 0 } };
	}

	static WString::View GetLine(WString::View view, CaretPtr caret_ptr)
	{
		return GetLineEx(view, caret_ptr).b;
	}

	static WString::View GetPreviousLine(const WString::View & view, const WString::View & line)
	{
		WString::View prev = { view.data, 0 };

		auto itr = view;

		while (itr)
		{
			auto i = Data::Detail::ReadLine(itr);

			if (i.data == line.data) break;

			prev = i;
		}

		return prev;
	}

	static bool GetNextLineEx(const WString::View & view, WString::View & line)
	{
		auto itr = Splice(view, UInt(line.data - view.data)).b;

		Data::Detail::ReadLine(itr);

		if (itr)
		{
			line = Data::Detail::ReadLine(itr);

			return true;
		}
		else
		{
			return false;
		}
	}

	static WString::View GetNextLine(const WString::View & view, const WString::View & line)
	{
		auto i = line;

		if (GetNextLineEx(view, i))
		{
			return i;
		}

		return { view.data + view.size, 0 };
	}

	REFLEX_INLINE static bool IsSymbol(WChar c)
	{
		if (c < 48) return true;

		if (c > 122) return true;

		switch (c)
		{
		case ':':
		case ';':  // ';'
		case '<':  // '<'
		case '>':  // '>'
		case '=':  // '='
		case '?':  // '?'
		case '@':  // '@'
		case '[':  // '['
		case '\\': // '\\'
		case ']':  // ']'
		case '^':  // '^'
		case '`':  // '`'
			return true;

		default:
			return false;
		}
	}

	template <bool AFTER> static CaretPtr Jump(const WString::View & text, CaretPtr caretpos)
	{
		enum Type : UInt
		{
			kTypeWhiteSpace,
			kTypeText,
			kTypeSymbol,
			kTypeLineBreak,
		};

		CaretPtr previous = text.data;

		auto type = UInt(kTypeWhiteSpace);

		auto linebreak = UInt(kTypeLineBreak);

		REFLEX_LOOP_PTR(text.data, itr, text.size)
		{
			auto t = UInt(kTypeText);

			if (*itr == kCR || *itr == kLF)
			{
				t = linebreak;

				if (*itr == kLF) linebreak++;
			}
			else if (IsWhiteSpace(*itr))
			{
				t = kTypeWhiteSpace;

				linebreak = kTypeLineBreak;
			}
			else if (IsSymbol(*itr))
			{
				t = kTypeSymbol;

				linebreak = kTypeLineBreak;
			}
			else
			{
				linebreak = kTypeLineBreak;
			}

			if (SetFiltered(type, t))
			{
				if (t)
				{
					if constexpr (AFTER)
					{
						if (itr > caretpos) return itr;
					}
					else
					{
						if (itr >= caretpos) return previous;
					}

					previous = itr;
				}
			}
		}

		if constexpr (AFTER)
		{
			return text.data + text.size;
		}
		else
		{
			return previous;
		}
	}

	AbstractViewPort * GetViewPort()
	{
		if (auto vp = QueryParentByType<AbstractViewPort>(object))
		{
			if (vp->GetContent() == object)
			{
				return vp;
			}
			else if (auto vpref = object->QueryProperty<Detail::LegacyWeakReferenceObject>(K32("ViewPort")))
			{
				return DynamicCast<AbstractViewPort>(vpref->value);
			}
		}

		return 0;
	}

	void SetState(Key32 state)
	{
		if (auto vp = GetViewPort()) vp->SetState(state);

		object->SetState(state);
	}

	void ClearState(Key32 state)
	{
		if (auto vp = GetViewPort()) vp->ClearState(state);

		object->ClearState(state);
	}


	bool HasFocus() const { return Core::desktop->GetFocus() == object; }

	void StartCaretBlink();


	void Activate();

	void Deactivate(bool commit);


	bool ClearSelection();

	void BlockInsert(const WString::View & view, bool shift);


	static WString::View GetTabSequence(UInt8 num_spaces) { return num_spaces ? kTabSpaces : WString::View(&kTabChar, 1); }

	static bool PressHome(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressEnd(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressLeft(TextEditBehaviourImpl & self, bool shift, bool ctrl_alt[2]);
	static bool PressRight(TextEditBehaviourImpl & self, bool shift, bool ctrl_alt[2]);
	static bool PressVerticalArrow(TextEditBehaviourImpl & self, decltype (&GetNextLine) next, bool shift);

	template <bool CUT> static bool PressC(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressV(TextEditBehaviourImpl & self, bool shift, bool ctrl);

	static bool PressA(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressD(TextEditBehaviourImpl & self, bool shift, bool ctrl);

	static bool PressK(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressZ(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressY(TextEditBehaviourImpl & self, bool shift, bool ctrl);

	static bool PressDelete(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressBackspace(TextEditBehaviourImpl & self, bool shift, bool platform_modifier);

	static bool PressEnter(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressTab(TextEditBehaviourImpl & self, bool shift, bool ctrl);
	static bool PressEscape(TextEditBehaviourImpl & self, bool shift, bool ctrl);

	static bool IgnoreAlt(decltype(&PressEscape) fn, TextEditBehaviourImpl & self, bool shift, bool ctrl, bool alt) { if (alt) { return false; } else { return fn(self, shift, ctrl); } }


	void SetCaretImpl(CaretPtr caret, bool select);

	void UpdateSelection();

	void Insert(const WString::View & string);

	void InsertCharacter(WChar character);

	UInt Remove(const WString::View & view, UInt16 pos, UInt length);


	CaretPtr GetCharacterAtPoint(const Point & point);


	static inline Idx st_dragstart;

	static inline WChar st_charflags = WChar(~0);
};

struct TextEditBehaviourImpl::TransactionScope
{
	struct State : public Reflex::Object
	{
		State(TextEditBehaviourImpl & self)
			: value(self.m_text->GetView()),
			caret(self.m_caret),
			selection(self.m_selection),
			count(0)
		{
		}

		WString value;

		UInt16 caret;

		Pair <UInt16> selection;

		UInt16 count;
	};

	TransactionScope(TextEditBehaviourImpl & self)
		: self(self),
		state(Data::Detail::AcquireProperty<State>(*self.object, kNullKey, self))
	{
		state.count++;
	}

	~TransactionScope()
	{
		if (!--state.count)
		{
			auto & target = *self.object;

			auto current_view = self.m_text->GetView();

			if (state.value != current_view)
			{
				WString current = current_view;	//view can change due to performtransaction

				Detail::PerformTransaction(target, kTextEdit);

				Data::Archive stream;

				stream.Allocate(16 + ((state.value.GetSize() + current.GetSize()) * 2));

				Data::SerializeUCS2(stream, state.value);

				Data::Serialize(stream, state.caret, state.selection);

				Data::SerializeUCS2(stream, current);

				Data::Serialize(stream, self.m_caret, self.m_selection);

				self.Commit(Data::Compress(Data::kLZ4, stream));
			}

			target.UnsetProperty<State>(kNullKey);

			if (current_view.size)
			{
				self.SetState(kUsedState);
			}
			else
			{
				self.ClearState(kUsedState);
			}

			target.Accommodate();

			self.Reveal();
		}
	}

	TextEditBehaviourImpl & self;

	State & state;
};

struct TextEditBehaviourImpl::ComputeScope
{
	ComputeScope(const TextEditBehaviourImpl & self)
		: self(self)
	{
		if (!self.m_compute_scope_count++)
		{
			self.object->ComputeLayout();
		}
	}

	~ComputeScope()
	{
		self.m_compute_scope_count--;
	}

	const TextEditBehaviourImpl & self;
};

struct TextEditBehaviourImpl::VirtualKeyboard : public Reflex::Object
{
	static TRef <VirtualKeyboard> Acquire(VirtualKeyboardInputType type, const WString::View & textbuffer, Pair<UInt> selection, const Function<void(const WString&, Pair<UInt>)>& ondone)
	{
		System::ShowVirtualKeyboard(type, textbuffer, selection, ondone);

		return The<VirtualKeyboard>::Acquire();
	}

	~VirtualKeyboard()
	{
		System::DismissVirtualKeyboard();
	}
};

void TextEditBehaviourImpl::SetInputType(VirtualKeyboardInputType type)
{
	m_input_type = type;

	if (HasFocus())
	{
		RequestTextInput();
	}
}

void TextEditBehaviourImpl::SetCaret(UInt position, UInt selection_start, UInt selection_length)
{
	auto length = UInt16(m_text->GetView().size);

	m_caret = Min(UInt16(position), length);

	selection_start = Min<UInt>(selection_start, length);

	selection_length = Min<UInt>(selection_length, length - selection_start);

	m_selection = { UInt16(selection_start), UInt16(selection_length) };

	UpdateSelection();
}

void TextEditBehaviourImpl::Update()
{
	auto view = m_text->GetView();

	m_caret = Min(m_caret, UInt16(view.size));

	m_selection = {};

	UpdateSelection();

	if (view)
	{
		SetState(kUsedState);
	}
	else
	{
		ClearState(kUsedState);
	}
}

void TextEditBehaviourImpl::Reveal()
{
	if (auto vp = GetViewPort())
	{
		ComputeScope compute(*this);

		auto actual = m_text->GetView();

		auto transformed = ToView(m_transformed);

		auto caret = TransformCaret(transformed, actual, actual.data + m_caret);

		auto [line, lineview] = GetLineEx(transformed, caret);

		lineview.size = Min(lineview.size, UInt(caret - lineview.data));

		Point position = m_text_rect.origin + MakePoint(m_font->GetTextWidth(lineview), line * m_lineh);

		vp->Reveal(false, position.x, 0.0f, 32.0f);

		vp->Reveal(true, position.y, m_lineh, 4.0f); //0.0f, 32.0f);
	}
}

Pair <Float> TextEditBehaviourImpl::GetLineCoordinates(UInt idx) const
{
	ComputeScope compute(*this);

	auto y = m_text_rect.origin.y;

	return { y + (m_lineh * idx), m_font->GetHeightAndTail().a };
}

void TextEditBehaviourImpl::Insert(const WString::View & string)
{
	TransactionScope t(*this);

	ClearSelection();

	auto value = m_text->GetView();

	auto pair = Splice(value, m_caret);

	m_text->SetValue(Join(pair.a, string, pair.b));

	m_caret += UInt16(string.size);

	Activate();
}

void TextEditBehaviourImpl::InsertCharacter(WChar character)
{
	Insert({ &character, 1 });
}

UInt TextEditBehaviourImpl::Remove(const WString::View & view, UInt16 pos, UInt length)
{
	TransactionScope t(*this);

	WString value = view;

	value.Remove(pos, length);

	m_text->SetValue(value);

	return value.GetSize();
}

void TextEditBehaviourImpl::RequestTextInput()
{
	auto selection = (m_selection.a || m_selection.b) ? MakeTuple(UInt(m_selection.a), UInt(m_selection.b)) : MakeTuple(UInt(m_caret), UInt(0));

	m_virtual_keyboard = VirtualKeyboard::Acquire
	(
		m_input_type,
		m_text->GetView(),
		selection,
		[this](const WString & newtext, Pair <UInt> selection)
		{
			Core::Context ctx;
			TransactionScope t(*this);

			m_text->SetValue(newtext);
			m_caret = UInt16(selection.a);
			m_selection = { UInt16(selection.a), UInt16(selection.b) };
		}
	);
}

void TextEditBehaviourImpl::StartCaretBlink()
{
	REFLEX_ASSERT(HasFocus());

	AttachPeriodicClock(object, kCaretBlink, 0.5f, [object = this->object, self = AutoRelease(this)]()
	{
		bool & flash = RemoveConst(self->m_show_caret);

		flash = !flash;

		object->Realign();
	});

	RemoveConst(m_show_caret) = true;
}

void TextEditBehaviourImpl::Activate()
{
	StartCaretBlink();


	auto text_view = m_text->GetView();

	auto length = UInt16(text_view.size);


	object->Accommodate();

	SetState(kActiveState);

	if (Detail::BeginTransaction(object, kTextEdit) && m_input_type != kVirtualKeyboardInputMultiLine)
	{
		m_caret = length;

		m_selection = { 0, length };

		if (object)
		{
			AttachAnimationClock(object, kClickBlocker, [object = this->object](Float)
			{
				DetachClock(object, kClickBlocker);
			});
		}
	}
	else
	{
		m_caret = Min(m_caret, length);

		m_selection.a = Min(m_selection.a, length);

		m_selection.b = UInt16(Clip<Int32>(m_selection.b, 0, length - Int32(m_selection.a)));
	}

	RequestTextInput();
}

void TextEditBehaviourImpl::Deactivate(bool commit)
{
	if (!commit)
	{
		while (CanUndo()) Undo();

		m_caret = 0;

		m_selection = {};
	}

	if (m_input_type != kVirtualKeyboardInputMultiLine)
	{
		m_caret = UInt16(m_text->GetView().size);

		m_selection = {};

		Reset();
	}

	object->Accommodate();

	Detail::EndTransaction(object, kTextEdit, !commit);

	OnDetachObject();
}

bool TextEditBehaviourImpl::ClearSelection()
{
	if (UInt length = m_selection.b)
	{
		TransactionScope t(*this);

		auto view = m_text->GetView();

		bool after = m_selection.a + length == view.size;

		auto left = Left<true>(view, m_selection.a);

		auto right = Right<true>(view, view.size - (m_selection.a + length));

		auto string = Join(left, right);

		m_text->SetValue(string);

		m_caret = Min<UInt16>(m_selection.a + after, UInt16(string.GetSize()));

		m_selection.b = 0;

		UpdateSelection();

		return true;
	}

	return false;
}

bool TextEditBehaviourImpl::PressHome(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	auto view = self.m_text->GetView();

	if (ctrl)
	{
		self.SetCaretImpl(view.data, shift);
	}
	else
	{
		auto line = GetLine(view, view.data + self.m_caret);

		self.SetCaretImpl(line.data, shift);
	}

	self.UpdateSelection();

	return true;
}

bool TextEditBehaviourImpl::PressEnd(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	auto view = self.m_text->GetView();

	if (ctrl)
	{
		self.SetCaretImpl(view.data + view.size, shift);
	}
	else
	{
		auto line = GetLine(view, view.data + self.m_caret);

		self.SetCaretImpl(line.data + line.size, shift);
	}

	self.UpdateSelection();

	return true;
}

bool TextEditBehaviourImpl::PressLeft(TextEditBehaviourImpl & self, bool shift, bool ctrl_alt[2])
{
	auto view = self.m_text->GetView();

	auto caret = view.data + self.m_caret;

	if (ctrl_alt[kApple])
	{
		caret = Jump<false>(view, caret);
	}
	else if (ctrl_alt[!kApple])
	{
		return false;
	}
	else
	{
		auto line = GetLine(view, caret);

		if (caret == line.data)
		{
			if (self.m_input_type == kVirtualKeyboardInputMultiLine) //&& !SetFiltered(caret, Max(line.data - 1, view.data)))
			{
				auto previous = GetPreviousLine(view, line);

				caret = previous.data + previous.size;
			}
			else
			{
				return false;
			}
		}
		else
		{
			caret--;
		}
	}

	self.SetCaretImpl(caret, shift);

	self.UpdateSelection();

	return true;
}

bool TextEditBehaviourImpl::PressRight(TextEditBehaviourImpl & self, bool shift, bool ctrl_alt[2])
{
	auto view = self.m_text->GetView();

	auto caret_ptr = view.data + self.m_caret;

	if (ctrl_alt[kApple])
	{
		caret_ptr = Jump<true>(view, caret_ptr);
	}
	else if (ctrl_alt[!kApple])
	{
		return false;
	}
	else
	{
		auto line = GetLine(view, caret_ptr);

		if (caret_ptr == line.data + line.size)
		{
			auto next = GetNextLine(view, line);

			caret_ptr = next.data;
		}
		else
		{
			caret_ptr++;
		}
	}

	self.SetCaretImpl(caret_ptr, shift);

	self.UpdateSelection();

	return true;
}

bool TextEditBehaviourImpl::PressVerticalArrow(TextEditBehaviourImpl & self, decltype (&GetNextLine) nextfn, bool shift)
{
	if (self.m_input_type == kVirtualKeyboardInputMultiLine)
	{
		auto view = self.m_text->GetView();

		auto caret = view.data + self.m_caret;

		auto line = GetLine(view, caret);

		auto next = nextfn(view, line);

		auto offset = caret - line.data;

		caret = Min(next.data + offset, next.data + next.size);

		self.SetCaretImpl(caret, shift);

		self.UpdateSelection();

		return true;
	}
	else
	{
		return false;
	}
}

template <bool CUT> bool TextEditBehaviourImpl::PressC(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl)
	{
		System::SetClipboard(Mid<false>(self.m_text->GetView(), self.m_selection.a, self.m_selection.b));

		if constexpr (CUT) self.ClearSelection();

		st_charflags = 0;
	}

	return true;
}

bool TextEditBehaviourImpl::PressV(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl)
	{
		TransactionScope t(self);

		self.Insert(System::GetClipboard());

		st_charflags = 0;
	}

	return true;
}

bool TextEditBehaviourImpl::PressA(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl)
	{
		self.m_caret = UInt16(self.m_text->GetView().size);

		self.m_selection = { 0, self.m_caret };

		st_charflags = 0;

		self.UpdateSelection();
	}

	return true;
}

bool TextEditBehaviourImpl::PressD(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl)
	{
		self.m_selection = {};

		self.UpdateSelection();

		st_charflags = 0;
	}

	return true;
}

bool TextEditBehaviourImpl::PressK(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl) self.BlockInsert(L"//", shift);

	return true;
}

bool TextEditBehaviourImpl::PressZ(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl)
	{
		if (shift && System::kPlatform == System::kPlatformMacOS)
		{
			self.Redo();
		}
		else
		{
			self.Undo();
		}

		self.Reveal();
	}

	return true;
}

bool TextEditBehaviourImpl::PressY(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (ctrl && System::kPlatform == System::kPlatformWindows)
	{
		self.Redo();

		self.Reveal();
	}

	return true;
}

bool TextEditBehaviourImpl::PressDelete(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	st_charflags = 0;

	if (!self.ClearSelection())
	{
		auto view = self.m_text->GetView();

		auto caret = view.data + self.m_caret;

		if (ctrl)
		{
			auto end = Jump<true>(view, caret);

			self.Remove(view, self.m_caret, UInt(end - caret));
		}
		else if (shift && self.m_input_type == kVirtualKeyboardInputMultiLine)
		{
			auto line = GetLine(view, caret);

			constexpr WChar endings[] = { kCR, kLF };

			auto max = view.data + view.size;

			for (auto & i : endings)
			{
				if ((line.data + line.size) < max && (line.data[line.size] == i)) line.size++;
			}

			self.m_caret = UInt16(line.data - view.data);

			self.Remove(view, self.m_caret, line.size);
		}
		else if (self.m_caret < view.size)
		{
			auto length = self.Remove(view, self.m_caret, 1);

			self.m_caret = Min(self.m_caret, UInt16(length));
		}
	}

	return true;
}

bool TextEditBehaviourImpl::PressBackspace(TextEditBehaviourImpl & self, bool shift, bool platform_modifier)
{
	st_charflags = 0;

	if (!self.ClearSelection())
	{
		auto view = self.m_text->GetView();

		if (view && self.m_caret)
		{
			UInt16 length = 1;

			auto caret_ptr = view.data + self.m_caret;

			if (platform_modifier)
			{
				auto t = Jump<false>(view, caret_ptr);

				length = UInt16(caret_ptr - t);
			}
			else if (self.m_tab_spaces)
			{
				auto tab = GetTabSequence(self.m_tab_spaces);

				auto line = GetLine(view, caret_ptr);

				if (auto indent = UInt(caret_ptr - line.data))
				{
					if (!(indent % tab.size))
					{
						for (auto c : Left(line, indent))
						{
							if (c != ' ') goto Remove;
						}

						length = UInt16(tab.size);
					}
				}
			}

			Remove:

			self.m_caret -= length;

			self.Remove(view, self.m_caret, length);
		}
	}

	return true;
}

bool TextEditBehaviourImpl::PressEscape(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (self.m_input_type != kVirtualKeyboardInputMultiLine)
	{
		Array <Core::WeakReference> parents;

		auto vp = self.GetViewPort();

		for (auto & i : GLX::Object::ParentRange(*(vp ? *vp : *self.object).GetParent())) parents.Push(i);

		self.Deactivate(false);

		for (auto & i : parents)
		{
			if (IsValid(*i))
			{
				i->Focus();

				break;
			}
		}

		return true;
	}

	return false;
}

bool TextEditBehaviourImpl::PressEnter(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	constexpr auto GetIndent = [](const WString::View & tab, const WString::View & line) -> WString::View
	{
		if (tab.size && line.size >= tab.size)
		{
			WString::View itr = { line.data, tab.size };

			auto end = line.data + (line.size - tab.size);

			while (itr.data <= end && itr == tab)
			{
				itr.data += tab.size;
			}

			return { line.data, UInt(itr.data - line.data) };
		}
		else
		{
			return { line.data, 0 };
		}
	};

	if (self.m_input_type == kVirtualKeyboardInputMultiLine)
	{
		if (ctrl)
		{
			Emit(self.object, kComplete);

			self.Deactivate(true);
		}
		else
		{
			auto view = self.m_text->GetView();

			auto caret = view.data + self.m_caret;

			auto line = GetLine(view, caret);

			auto tab = GetTabSequence(self.m_tab_spaces);

			auto indent = GetIndent(tab, line);

			auto caret_indent = UInt(caret - line.data);

			if (caret_indent < indent.size && Mid<true>(line.data, caret_indent, tab.size) == tab)
			{
				indent.size = caret_indent;
			}

			self.Insert(Join(kLF, indent));
		}
	}
	else
	{
		Emit(self.object, kComplete);

		self.Deactivate(true);
	}

	return true;
}

void TextEditBehaviourImpl::BlockInsert(const WString::View & string, bool shift)
{
	TransactionScope t(*this);

	if (m_input_type == kVirtualKeyboardInputMultiLine)
	{
		const auto view = m_text->GetView();

		WString copy = view;

		auto start = view.data;

		if (m_selection.b)
		{
			auto end = start + m_selection.a + m_selection.b;

			auto line = GetLine(view, start + m_selection.a);

			auto last = GetLine(view, end);

			if (line != last)	//block indent
			{
				UInt idx = 0;

				if ((m_selection.a + m_selection.b) > (last.data - start))
				{
					GetNextLineEx(view, last);
				}

				while (line.data < end)
				{
					auto pos = UInt(line.data - start);

					if (shift)
					{
						if (Mid<false>(view, pos, string.size) == string)
						{
							copy.Remove(pos - idx, string.size);

							idx += string.size;

							m_selection.b -= UInt16(string.size);
						}

						//TODO auto adjust caret
					}
					else
					{
						for (auto & i : string) copy.Insert(pos + idx, i);

						idx += string.size;

						m_selection.b += UInt16(string.size);

						if ((line.data - start) > m_caret) m_caret++;
					}

					if (!GetNextLineEx(view, line))
					{
						break;
					}
				}

				m_caret = Min(m_selection.a, UInt16(copy.GetSize()));

				m_text->SetValue(copy);

				UpdateSelection();
			}
		}
		else if (shift)
		{
			if (m_caret < (Int(view.size) - Int(string.size)))
			{
				if (Mid<false>(view, m_caret, string.size) == string)
				{
					copy.Remove(m_caret, string.size);
				}
			}
		}
		else
		{
			for (auto & i : string) copy.Insert(m_caret++, i);
		}

		m_text->SetValue(copy);
	}
}

bool TextEditBehaviourImpl::PressTab(TextEditBehaviourImpl & self, bool shift, bool ctrl)
{
	if (!ctrl && self.m_input_type == kVirtualKeyboardInputMultiLine)
	{
		self.BlockInsert(GetTabSequence(self.m_tab_spaces), shift);

		return true;
	}
	else
	{
		return false;
	}
}

void TextEditBehaviourImpl::SetCaretImpl(CaretPtr caretptr, bool select)
{
	if (select)
	{
		auto caret = UInt16(caretptr - m_text->GetView().data);

		if (m_selection.b)
		{
			Int delta = caret - Int(m_selection.a);

			if (delta > 0)
			{
				m_selection.b = UInt16(delta);
			}
			else //if (data.caret < caret)
			{
				m_selection.a = caret;

				m_selection.b -= UInt16(delta);
			}
		}
		else
		{
			if (m_caret > caret)
			{
				m_selection = { caret, UInt16(m_caret - caret) };
			}
			else
			{
				m_selection = { m_caret, UInt16(caret - m_caret) };
			}
		}
	}
	else
	{
		m_selection = {};
	}

	m_caret = UInt16(caretptr - m_text->GetView().data);

	Reveal();
}

void TextEditBehaviourImpl::UpdateSelection()
{
	if (m_virtual_keyboard) RequestTextInput();

	object->Realign();
}

void TextEditBehaviourImpl::OnAttachObject()
{
	m_text = Data::Detail::AcquireProperty<Text>(object, m_textid, false);

	auto multi_line = m_text->IsMultiLine();

	m_input_type = multi_line ? kVirtualKeyboardInputMultiLine : kVirtualKeyboardInputNormal;

	History::SetCapacity(multi_line ? (kMaxUInt16 * 64) : kMaxUInt16);

	if (HasFocus())
	{
		Activate();
	}
}

void TextEditBehaviourImpl::OnDetachObject()
{
	RemoveConst(m_show_caret) = false;

	DetachClock(object, kClickBlocker);

	DetachClock(object, kCaretBlink);

	ClearState(kActiveState);

	m_virtual_keyboard.Clear();
}

bool TextEditBehaviourImpl::OnEvent(GLX::Object & src, Event & e)
{
	if (e.id == kFocus)
	{
		//TODO ignore kMouseWheel events

		if (!kIsMobile || m_input_type != kVirtualKeyboardInputMultiLine) Activate();

		return true;
	}
	else if (e.id == kLoseFocus)
	{
		auto keyboard_retainer = m_virtual_keyboard;	//keep keyboard alive for 1 more frame, to filter out Dismiss/Show cycles

		auto require_enter = Detail::GetBool(object, K32("require_enter"));

		Deactivate(!require_enter);			//m_virtual_keyboard cleared in Deactivate

		AttachAnimationClock(object, kKeepKeyboardAlive, [object = this->object, keyboard_retainer](Float)
		{
			DetachClock(object, kKeepKeyboardAlive);
		});

		return true;
	}
	else if (e.id == kMouseDown)
	{
		auto flags = GetClickFlags(e);

		if (Not(flags & (kClickFlagRmb | kPointerFlagMulti)))
		{
			if (HasFocus())
			{
				StartCaretBlink();

				if (flags & kClickFlagDbl)
				{
					auto view = m_text->GetView();

					auto caretptr = view.data + m_caret;

					SetCaretImpl(Jump<true>(view, caretptr), false);

					SetCaretImpl(Jump<false>(view, caretptr), true);

					WString::View selection = { view.data + m_selection.a, m_selection.b };

					auto trimmed = TrimRight(selection);

					m_selection.b = UInt16(trimmed.size);

					UpdateSelection();
				}
				else if (!QueryAbstractProperty(object, kClickBlocker))
				{
					auto caretptr = GetCharacterAtPoint(GetPointerPosition(object, e));

					if (GetModifierKeys(e) & kModifierKeyShift)
					{
						st_dragstart = m_selection.b ? m_selection.a : m_caret;

						SetCaretImpl(caretptr, true);
					}
					else
					{
						SetCaretImpl(caretptr, false);

						st_dragstart = m_caret;
					}

					UpdateSelection();
				}
			}

			return true;
		}
	}
	else if (e.id == kMouseDrag)
	{
		if (st_dragstart)
		{
			auto caret = UInt16(GetCharacterAtPoint(GetPointerPosition(object, e)) - m_text->GetView().data);

			auto origin = UInt16(st_dragstart.value);

			if (origin > caret)
			{
				Swap(origin, caret);
			}

			m_selection = { origin, UInt16(caret - origin) };

			m_caret = caret;

			UpdateSelection();
		}

		return true;
	}
	else if (e.id == kMouseUp)
	{
		st_dragstart = {};

		return true;
	}
	else if (e.id == kKeyDown)
	{
		StartCaretBlink();	//sync

		auto & self = *this;

		auto modifiers = GetModifierKeys(e);

		bool shift = modifiers & kModifierKeyShift;
		bool ctrl = modifiers & kModifierKeyPrimary;
		bool alt = modifiers & kModifierKeyAlt;
		bool system = modifiers & kModifierKeySystem;

		bool ctrl_alt[2] = { ctrl, alt };

		bool platform_modifier = ctrl_alt[System::kPlatform == System::kPlatformMacOS];

		st_charflags = Reinterpret<WChar>(kMaxUInt32);

		switch (GetKeyCode(e))
		{
		case kKeyCodeLeft:
			if (system)
			{
				return PressHome(self, shift, false);
			}
			else
			{
				return PressLeft(self, shift, ctrl_alt);
			}

		case kKeyCodeRight:
			if (system)
			{
				return PressEnd(self, shift, false);
			}
			else
			{
				return PressRight(self, shift, ctrl_alt);
			}

		case kKeyCodeUp:
			if (system)
			{
				return PressHome(self, shift, true);
			}
			else
			{
				return PressVerticalArrow(self, &TextEditBehaviourImpl::GetPreviousLine, shift);
			}

		case kKeyCodeDown:
			if (system)
			{
				return PressEnd(self, shift, true);
			}
			else
			{
				return PressVerticalArrow(self, &TextEditBehaviourImpl::GetNextLine, shift);
			}

		case kKeyCodeA: return PressA(self, shift, ctrl);
		case kKeyCodeD: return PressD(self, shift, ctrl);
		case kKeyCodeC: return PressC<false>(self, shift, ctrl);
		case kKeyCodeX: return PressC<true>(self, shift, ctrl);
		case kKeyCodeV: return PressV(self, shift, ctrl);

		case kKeyCodeK:
		case kKeyCodeSlash:
			return PressK(self, shift, ctrl);

		case kKeyCodeZ: return PressZ(self, shift, ctrl);
		case kKeyCodeY: return PressY(self, shift, ctrl);

		case kKeyCodeHome: return IgnoreAlt(&PressHome, self, shift, ctrl, alt);
		case kKeyCodeEnd: return IgnoreAlt(&PressEnd, self, shift, ctrl, alt);

		case kKeyCodeDelete: return IgnoreAlt(&PressDelete, self, shift, ctrl, alt);
		case kKeyCodeBackspace: return PressBackspace(self, shift, platform_modifier);
		case kKeyCodeEnter: return IgnoreAlt(&PressEnter, self, shift, ctrl, alt);
		case kKeyCodeTab: return IgnoreAlt(&PressTab, self, shift, ctrl, alt);
		case kKeyCodeEscape: return IgnoreAlt(&PressEscape, self, shift, ctrl, alt);

		case kKeyCodePageUp:
		case kKeyCodePageDown:
		case kKeyCodeF1:
		case kKeyCodeF2:
		case kKeyCodeF3:
		case kKeyCodeF4:
		case kKeyCodeF5:
		case kKeyCodeF6:
		case kKeyCodeF7:
		case kKeyCodeF8:
		case kKeyCodeF9:
		case kKeyCodeF10:
		case kKeyCodeF11:
		case kKeyCodeF12:
			return false;

		default:
			if (alt)
			{
				return true;// !(ctrl || alt);
			}
			else
			{
				return !ctrl;
			}
		};
	}
	else if (e.id == kCharacter)
	{
		if (auto character = WChar(GetKeyCharacter(e) & st_charflags))
		{
			if (character > 31) InsertCharacter(character);

			return true;
		}
	}

	return false;
}

TextEditBehaviourImpl::CaretPtr TextEditBehaviourImpl::GetCharacterAtPoint(const Point & point)
{
	ComputeScope compute(*this);

	auto actual = m_text->GetView();

	auto position = m_text_rect.origin;

	auto all = ToView(m_transformed);

	auto view = all;

	while (view)
	{
		auto line = Data::Detail::ReadLine(view);

		if (point.y < (position.y + m_lineh))
		{
			UInt n = line.size;

			line.size = 1;

			while (line.size <= n)
			{
				Float w = m_font->GetTextWidth(line);

				if (point.x < position.x + w)
				{
					return TransformCaret(actual, all, line.data + line.size - 1);
				}

				line.size++;
			}

			return TransformCaret(actual, all, line.data + n);
		}

		position.y += m_lineh;
	}

	return TransformCaret(actual, all, view.data + view.size);
}

void TextEditBehaviourImpl::OnRestoreHistory(Data::Archive::View stream, bool redo)
{
	auto archive = Data::Decompress(Data::kLZ4, stream);

	auto substream = ToView(archive);

	WString value;

	REFLEX_LOOP(idx, redo ? 2 : 1)
	{
		Data::DeserializeUCS2(substream, value);

		Data::Deserialize(substream, m_caret, m_selection);
	}

	m_text->SetValue(value);

	object->Accommodate();

	Detail::PerformTransaction(object, kTextEdit);

	UpdateSelection();
}

REFLEX_END_INTERNAL

Reflex::GLX::TextEditBehaviourWithState::TextEditBehaviourWithState(Key32 textid)
	: m_textid(textid)
	, m_input_type(kVirtualKeyboardInputNormal)
	, m_show_caret(false)
	, m_caret(0)
	, m_lineh(0.0f)
{
}

Reflex::TRef <Reflex::GLX::TextEditBehaviour> Reflex::GLX::TextEditBehaviour::Create(Key32 text_id)
{
	return REFLEX_CREATE(TextEditBehaviourImpl, text_id);
}

Reflex::TRef <Reflex::Object> Reflex::GLX::AcquireVirtualKeyboard()
{
	return TextEditBehaviourImpl::VirtualKeyboard::Acquire(System::kVirtualKeyboardInputNormal, {}, {}, {});
}
