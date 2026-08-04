#include "console.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

REFLEX_DECLARE_KEY32(Search);
REFLEX_DECLARE_KEY32(SearchCaret);

#if (defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_IOS))
static constexpr bool kApple = true;
#else
static constexpr bool kApple = false;
#endif

constexpr Key32 kConfig = "Config";
constexpr Pair <Key32,bool> kLineNumbers = { "ide.texteditor.line_numbers", false };
constexpr Pair <Key32,UInt8> kTabSpaces = { "ide.texteditor.tab_spaces", 4 };
constexpr Pair <Key32,bool> kSearchOptionCaseSensitive = { "ide.texteditor.search.case_sensitive", false };
constexpr Pair <Key32,bool> kSearchOptionWholeWord = { "ide.texteditor.search.word", false };

template <class TYPE> TYPE GetPreference(Key32 id, TYPE fallback)
{
	if constexpr (kIsType<TYPE,bool>)
	{
		return Data::GetBool(TheGlobal::Get()->m_prefs, id, File::UnpackResource<bool>(kConfig, id, fallback));
	}
	else if constexpr (kIsType<TYPE,UInt8>)
	{
		return Data::GetUInt8(TheGlobal::Get()->m_prefs, id, File::UnpackResource<UInt8>(kConfig, id, fallback));
	}
	else
	{
		static_assert(kIsType<TYPE,void>);	//always fail
	}
}

auto GetPreference(auto def)
{
	return GetPreference<decltype(def.b)>(def.a, def.b);
}

void SetPreference(auto def, auto value)
{
	if constexpr (kIsType<decltype(def.b), bool>)
	{
		Data::SetBool(TheGlobal::Get()->m_prefs, def.a, True(value));
	}
	else if constexpr (kIsType<decltype(def.b), UInt8>)
	{
		Data::SetUInt8(TheGlobal::Get()->m_prefs, def.a, UInt8(value));
	}
	else
	{
		static_assert(kIsType<decltype(def.b), void>);
	}
}

template <bool REVERSE, bool CASE_SENSITIVE, bool WHOLE_WORD> Idx TextSearch(const WString & haystack, const WString::View & needle, UInt pos)
{
	typedef ConditionalType <CASE_SENSITIVE,CaseSensitive,CaseInsensitive> CaseSensitivity;
	
	Idx result;

	if constexpr (REVERSE)
	{
		if (auto idx = ReverseSearch<CaseSensitivity>(Left<true>(haystack, pos), needle))
		{
			result = idx.value;
		}
	}
	else if (auto idx = Search<CaseSensitivity>(Mid<true>(haystack, pos), needle))
	{
		result = idx.value + pos;
	}

	if constexpr (WHOLE_WORD)
	{
		if (result)
		{
			bool left = true;

			bool right = true;

			if (result.value) left = !Data::Detail::IsAlphaNumericCharacter(char(haystack[result.value - 1]));

			auto end = result.value + needle.size;

			if (end < haystack.GetSize()) right = !Data::Detail::IsAlphaNumericCharacter(char(haystack[end]));

			return (left && right) ? result : Idx();
		}
		else
		{
			return result;
		}
	}
	else
	{
		return result;
	}
}

REFLEX_TBINDER_3P(TextSearch);

void EnumerateClients(const Function <void(const WString &, const ResourceGroupImpl&)> & visitor)
{
	for (auto & i : TheGlobal::Get()->m_clients)
	{
		visitor(i.value.a, i.value.b);
	}
}

struct BuilderImpl : public Detail::BuilderPanel
{
	struct Interface;

	struct TextEditor;

	struct BinaryEditor;

	struct PropertySheetEditor;


	BuilderImpl();

	~BuilderImpl();


	void RegisterWriteableDomain(Key32 domain) override
	{
		m_writeable_domains.Set(domain);
	}

	bool SelectFile(Address address) override
	{
		for (auto & i : m_files)
		{
			if (Search(i.value, address)) return true;
		}
	
		if (auto token = FindToken(address))
		{
			Open(address, true);
		}

		return false;
	}


	void RequestReload()
	{
		TheGlobal::Get()->Notify();
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override;

	void OnStore(Data::Archive & stream) const override;


	bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	void OnClock(Float32) override;

	void OnUpdate() override;

	void OnAttachWindow() override;


	const Map < WString, Array <Address> > & GetFilesByGroup();

	Array <Address> GetFilesOrdered();

	static ConstReference <Data::PropertySet> GetError(Reflex::Object & object);

	TRef <Interface> AcquireEditor(Key32 type);

	bool Open(Address address, bool focus);

	void Save();

	void OpenInExternalEditor();

	void Cycle(Int32 inc);


	const File::ResourcePool::Token * FindToken(Address address) const
	{
		File::ResourcePool::Lock lock(TheGlobal::Get()->m_resourcepool);

		if (auto token = lock.Query(address))
		{
			//TODO not sure why this happens, this is a temp workaround

			if (token->attributes.resolved_path) return token;
		}

		return 0;
	}

	Tuple < ConstReference <Reflex::Object>, Data::Archive, Data::Archive > & AcquireState(Key32 id)
	{
		return m_state[id];
	}

	const Tuple < ConstReference <Reflex::Object>,Data::Archive,Data::Archive > & GetState(Key32 id) const
	{
		return *m_state.Search(id, &m_null_state);
	}



	ConstReference <Global> m_ide_global;

	State::Monitor m_monitor;


	Map <TypeID,Key32> m_supported_types;

	Map <Key32> m_writeable_domains;


	ConstReference <GLX::Style> m_ide_styles;

	const GLX::Style * m_item;


	Map < WString, Array <Address> > m_files;

	Pair <Address, Reference <Reflex::Object> > m_content;

	mutable Map < Key32, Tuple <ConstReference<Reflex::Object>,Data::Archive,Data::Archive> > m_state;

	bool m_islocal;


	GLX::Object m_header;

	GLX::Popup m_source_selector;

	GLX::Button m_save, m_edit;


	Reference <Interface> m_editor;


	const Tuple <ConstReference<Reflex::Object>,Data::Archive,Data::Archive> m_null_state;
};

struct BuilderImpl::Interface : public GLX::Object
{
	static constexpr Key32 kType = K32("");


	Interface(TRef <BuilderImpl> builder)
		: builder(builder),
		m_needs_commit(false)
	{
	}

	void Activate(bool enable)
	{
		if (enable)
		{
			GLX::UnsetOpacity(*this, kNullKey);
		}
		else
		{
			GLX::SetOpacity(*this, kNullKey, 0.4f, GLX::Detail::ComputedStyle::kRenderFalse);
		}

		OnActivate(enable);
	}

	void SetContent(Key32 id, const Reflex::Object & data) 
	{
		GLX::Object::id = id;
		
		OnSetContent(data);

		Update();
	}

	void ClearContent()
	{
		OnClearContent();
	}


	virtual Key32 GetType() const { return kNullKey; }

	virtual void OnSetContent(const Reflex::Object & data) {}

	virtual void OnClearContent() {}

	virtual Data::Archive GetBlob() { return {}; }

	virtual void ClearError() {}

	virtual void ShowError(const Reflex::Object & error) {}

	virtual void OnActivate(bool enable)
	{
		GLX::EnableMouse(*this, enable, !enable);

		GLX::RedirectFocus(GetWindow()->GetContent(), *this);
	}

	virtual void Reset() {}

	virtual void Deserialize(bool session, Data::Archive::View stream) {}

	virtual void Serialize(bool session, Data::Archive & stream) const {}

	void Store() const
	{
		if (id != kNullKey)
		{
			auto parchive = &builder.AcquireState(id).b;

			REFLEX_LOOP(idx, 2)
			{
				auto & archive = parchive[idx];

				archive.Clear();

				Serialize(True(idx), archive);
			}
		}
	}



protected:

	friend struct BuilderImpl;

	void CommitState(const Reflex::Object & content)
	{
		REFLEX_ASSERT(id != kNullKey);

		m_needs_commit = true;

		builder.AcquireState(id).a = content;
	}


	BuilderImpl & builder;

	bool m_needs_commit;
};

struct BuilderImpl::TextEditor : public Interface
{
	static constexpr UInt32 kType = K32("text");


	TextEditor(TRef <BuilderImpl> builder);

	Key32 GetType() const override { return kType; }

	void OnSetContent(const Reflex::Object & data) override;

	Data::Archive GetBlob() override;

	void ClearError() override;

	void ShowError(const Reflex::Object & error) override;

	void OnActivate(bool enable) override
	{
		auto content = m_scroller.GetContent();

		GLX::EnableMouse(content, enable, !enable);

		GLX::RedirectFocus(GetWindow()->GetContent(), m_texteditor);
	}

	void Reset() override
	{
		m_texteditor->behaviour->Reset();
	}

	void Deserialize(bool session, Data::Archive::View stream) override
	{
		if (session)
		{
			m_texteditor->behaviour->Deserialize(stream);
		}
		else
		{
			auto [caret, selection_start, selection_length] = Data::Deserialize<UInt,UInt,UInt>(stream);
			
			m_texteditor->behaviour->SetCaret(caret, selection_start, selection_length);

			m_scroller.SetView({ Data::Deserialize<GLX::Point>(stream), {} });
		}
	}

	void Serialize(bool session, Data::Archive & stream) const override
	{
		if (session)
		{
			m_texteditor->behaviour->Serialize(stream);
		}
		else
		{
			auto vo = m_scroller.GetView().origin;

			Data::Serialize(stream, m_texteditor->behaviour->GetCaret(), vo);
		}
	}



protected:

	bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	void ShowLineNumbers(bool enable);

	TRef <InfoItem> AcquireFooter(Key32 id, const WString::View & label, const Function <void(GLX::Object&)> & populate = {})
	{
		return Cast<InfoItem>(GLX::Acquire(*this, id, GLX::kEnterAnimationSize, [this, &label, populate]
		{
			auto item = Init(New<InfoItem>(label), builder.m_ide_styles[K32("InfoItem")]);

			populate(item->value);

			return Cast<GLX::Object>(item);
		}));
	}

	struct TextEditorWithLineNumbers : public GLX::Object
	{
		TextEditorWithLineNumbers()
			: GLX::Object(GLX::Detail::kStandardLayoutRealigningContent)
		{
			linenumbers.SetPopulateCallback([this](UInt line, ArrayRegion < Reference <Object> > items, const GLX::Style & style)
			{
				for (auto & i : items)
				{
					i = GLX::Init(New<GLX::Label>(ToWString(++line)), style);
				}
			});

			ShowLineNumbers(GetPreference(kLineNumbers));

			GLX::AddInlineFlex(*this, texteditor);

			GLX::BindEvent(*this, GLX::kFocus, [this](GLX::Object & src, GLX::Event & e)
			{
				texteditor.Focus();

				return true;
			});
		}

		void ShowLineNumbers(bool enable)
		{
			if (enable)
			{
				auto data = texteditor.GetData();

				auto textview = Data::Unpack<CString::View>(data->value);

				UInt nline = 0;

				while (textview)
				{
					Data::Detail::ReadLine(textview);

					nline++;
				}

				linenumbers.SetNumItem(nline);

				GLX::AddInline(*this, linenumbers)->SendBottom();
			}
			else
			{
				linenumbers.Detach();
			}
		}

		void OnSetStyle(const GLX::Style & style) override
		{
			linenumbers.SetStyle(style["LineNumbers"]);

			texteditor.SetStyle(style["Text"]);
		}


		GLX::VirtualList linenumbers;

		Detail::TextEditor texteditor;
	};

	GLX::ScrollerOfType <TextEditorWithLineNumbers> m_scroller;

	const TRef <Detail::TextEditor> m_texteditor;
};

struct BuilderImpl::PropertySheetEditor : public TextEditor
{
	static constexpr UInt32 kType = K32("propertysheet");

	using TextEditor::TextEditor;

	Key32 GetType() const override { return kType; }

	void OnSetContent(const Reflex::Object & data) override
	{
		if (auto propertysheet = DynamicCast<Data::PropertySet>(data))
		{
			m_propertysheet = propertysheet;

			File::ResourcePool::Lock lock(TheGlobal::Get()->m_resourcepool);

			try
			{
				lock.Enumerate(m_propertysheet->object_t->type_id, [adr = &data](const File::ResourcePool::Token & token)
				{
					if (token.object.Adr() == adr) throw(&token);
				});
			}
			catch (const File::ResourcePool::Token * token)
			{
				TextEditor::OnSetContent(New<Data::ArchiveObject>(File::Open(lock.lock, token->path)));

				return;
			}
		}

		if (auto blob = DynamicCast<Data::ArchiveObject>(data))
		{
			TextEditor::OnSetContent(*blob);
		}
		else
		{
			TextEditor::ClearContent();
		}
	}

	ConstReference <Data::PropertySet> m_propertysheet;
};

BuilderImpl::BuilderImpl()
	: Detail::BuilderPanel("Builder", 1)
	, m_ide_global(TheGlobal::Get())
	, m_monitor(m_ide_global)
	, m_ide_styles(Detail::RetrieveStyleSheet())
	, m_islocal(false)
	, m_editor(REFLEX_CREATE(Interface, this))
{
	REFLEX_ASSERT(TheGlobal::IsAwake());


	m_writeable_domains.Set(kNullKey);

	m_supported_types.Set(GetTypeID<Data::ArchiveObject>(), TextEditor::kType);
	m_supported_types.Set(GetTypeID<GLX::StyleSheet>(), PropertySheetEditor::kType);
	m_supported_types.Set(GetTypeID<Data::PropertySet>(), PropertySheetEditor::kType);


	GLX::SetText(m_save, L"Save");

	GLX::SetText(m_edit, L"Edit");


	m_header.SetStyle(m_ide_styles["Bar"]);

	m_source_selector.SetStyle(m_ide_styles["Popup"]);

	auto button = m_ide_styles["Button"];

	m_save.SetStyle(button);

	m_edit.SetStyle(button);


	GLX::SetFlow(*this, GLX::kFlowY);

	Data::SetBool(*this, GLX::kresizable, true);

	GLX::EnableAutoFit(m_source_selector, false, true);

	GLX::AddInlineFlex(m_header, m_source_selector);

	GLX::AddInline(m_header, m_save);

	GLX::AddInline(m_header, m_edit);

	GLX::AddInline(*this, m_header);


	EnableOnAttachDetachWindow();

	EnableOnClock();

	Update();
}

BuilderImpl::~BuilderImpl()
{
	Clear();
}

const Map < WString, Array <Address> > & BuilderImpl::GetFilesByGroup()
{
	if (!m_files)
	{
		File::ResourcePool::Lock lock(TheGlobal::Get()->m_resourcepool);

		Map <Key32> used;

		EnumerateClients([this, &lock, &used](const WString & group, const ResourceGroupImpl & resources)
		{
			auto & files = m_files[group];

			for (auto & i : resources.m_items)
			{
				auto token = lock.Query(i.adr);

				if (token && m_supported_types.Search(i.adr.type_id))
				{
					if (!Search(files, i.adr)) files.Push(i.adr);
				}

				used[i.adr.id];
			}
		});
	}

	return m_files;
}

Array <Address> BuilderImpl::GetFilesOrdered()
{
	GetFilesByGroup();

	Array <Address> files;

	for (auto & group : m_files)
	{
		for (auto & i : group.value)
		{
			files.Push(i);
		}
	}

	return files;
}

ConstReference <Data::PropertySet> BuilderImpl::GetError(Reflex::Object & object)
{
	return GetProperty<Data::PropertySet>(object, Data::kError);
}

TRef <BuilderImpl::Interface> BuilderImpl::AcquireEditor(Key32 type)
{
	auto & editor = *m_editor;

	if (editor.GetType() != type)
	{
		editor.Store();

		editor.Detach();

		switch (type.value)
		{
		case TextEditor::kType:
			m_editor = New<TextEditor>(this);
			break;

		case PropertySheetEditor::kType:
			m_editor = New<PropertySheetEditor>(this);
			break;

		default:
			m_editor = New<Interface>(this);
			break;
		};

		GLX::AddInlineFlex(*this, m_editor);
	}

	m_editor->Reset();

	return m_editor;
}

bool BuilderImpl::Open(Address address, bool focus)
{
	if (m_content.a != address)
	{
		if (auto ptype = m_supported_types.Search(address.type_id))
		{
			if (auto token = FindToken(address))
			{
				auto object = token->object;

				m_content = { address, object };

				m_islocal = System::IsAbsolutePath(token->attributes.resolved_path);

				GLX::Activate(m_save, m_islocal);

				AcquireEditor(*ptype);

				Update();

				return true;
			}
		}
	}

	m_editor->ClearContent();

	return false;
}

void BuilderImpl::Save()
{
	if (auto a = FindToken(m_content.a))
	{
		m_editor->Store();

		File::VirtualFileSystem::Lock lock(TheGlobal::Get()->m_resourcepool->filesystem);

		auto blob = m_editor->GetBlob();

		if (!File::Save(lock, a->path, blob))
		{
			File::Save(lock, a->attributes.resolved_path, blob);
		}

		RequestReload();
	}
}

void BuilderImpl::OpenInExternalEditor()
{
	if (auto a = FindToken(m_content.a))
	{
		if (System::IsAbsolutePath(a->attributes.resolved_path))
		{
			System::Open(a->attributes.resolved_path);
		}
	}
}

void BuilderImpl::Cycle(Int32 inc)
{
	auto files = GetFilesOrdered();

	if (auto pidx = Search(files, m_content.a))
	{
		Int idx = Modulo<Int>(Int(pidx.value) + inc, files.GetSize());

		Open(files[idx], true);

		m_editor->Focus();
	}
}

void BuilderImpl::OnRestore(Data::Archive::View & stream, Key32 context)
{
	constexpr auto FindInResourcePool = [](BuilderImpl * self, const WString & path) -> Address
	{
		File::ResourcePool::Lock lock(TheGlobal::Get()->m_resourcepool);

		for (auto & i : self->m_supported_types)
		{
			if (auto token = lock.Query({ path, i.key }))
			{
				return token->address;
			}
		}

		return {};
	};

	WString path = Data::DeserializeUCS2(stream);

	Open(FindInResourcePool(this, path), true);

	REFLEX_LOOP(idx, Data::Deserialize<UInt>(stream))
	{
		auto [id, bytes] = Data::Deserialize<Pair<Key32, UInt>>(stream);

		auto & state = AcquireState(id);

		state.b = Data::ReadBytes(stream, bytes);

		state.c.Clear();
	}
}

void BuilderImpl::OnStore(Data::Archive & stream) const
{
	m_editor->Store();

	if (auto token = FindToken(m_content.a))
	{
		Data::SerializeUCS2(stream, token->path);
	}
	else
	{
		Data::SerializeUCS2(stream, {});
	}

	Data::Marker <UInt> marker(stream);

	UInt n = 0;

	for (auto & group : m_files)
	{
		for (auto & i : group.value)
		{
			if (auto pitem = m_state.Search(i.id))
			{
				Data::Serialize(stream, i.id, pitem->b);

				n++;
			}
		}
	}

	marker.Set(n);
}

bool BuilderImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (auto menu = GLX::GetMenu(e))
	{
		auto menusection = m_ide_styles[K32("menu")]["section"];

		auto groups = GetFilesByGroup();

		for (auto & group : groups)
		{
			if (group.value)
			{
				menu->AddSeparator(Init(REFLEX_CREATE(GLX::Label, group.key), menusection));

				for (auto & i : group.value)
				{
					if (auto token = FindToken(i))
					{
						auto item = menu->AddItem(token->attributes.resolved_path);

						Data::SetKey32(item, K32("overflow"), kOverflowPath);

						item->SetProperty(kNullKey, REFLEX_CREATE(ObjectOf<Address>, i));

						if (GetError(token->object))
						{
							AddFloat(item, GLX::Init(REFLEX_CREATE(GLX::Object), item->GetStyle()["error"]), GLX::kAlignmentRight);
						}

						if (i == m_content.a) item->Focus();
					}
				}
			}
		}

		GLX::BindEvent(*menu, GLX::Menu::kMenuSelect, [this](GLX::Object & src, GLX::Event & e)
		{
			auto item = GLX::GetItem(e);

			if (auto paddress = item->QueryProperty<ObjectOf<Address>>(kNullKey))
			{
				Open(paddress->value, true);

				m_editor->Focus();
			}

			return true;
		});

		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (src == m_save)
		{
			Save();

			return true;
		}
		else if (src == m_edit)
		{
			OpenInExternalEditor();

			return true;
		}
	}
	else if (e.id == GLX::kKeyDown)
	{
		switch (GLX_KEY_CODE(GLX::GetKeyCode(e), GLX::GetModifierKeys(e)))
		{
		case GLX_KEY_CODE(GLX::kKeyCodeLeft, (kApple ? GLX::kModifierKeyPrimary : GLX::kModifierKeyAlt)):
		case GLX_KEY_CODE(GLX::kKeyCodeTab, GLX::kModifierKeyPrimary):
		case GLX_KEY_CODE(GLX::kKeyCodeBracketClose, GLX::kModifierKeyShift | GLX::kModifierKeyPrimary):
			Cycle(-1);
			return true;

		case GLX_KEY_CODE(GLX::kKeyCodeRight, (kApple ? GLX::kModifierKeyPrimary : GLX::kModifierKeyAlt)):
		case GLX_KEY_CODE(GLX::kKeyCodeTab, GLX::kModifierKeyShift | GLX::kModifierKeyPrimary):
		case GLX_KEY_CODE(GLX::kKeyCodeBracketOpen, GLX::kModifierKeyShift | GLX::kModifierKeyPrimary):
			Cycle(1);
			return true;

		case GLX_KEY_CODE(GLX::kKeyCodeO, GLX::kModifierKeyAlt):
			m_source_selector.behaviour->Open();
			return true;

		case GLX_KEY_CODE(GLX::kKeyCodeS, GLX::kModifierKeyPrimary):
			Save();
			return true;
		}
	}

	return ConsolePanel::OnEvent(src, e);
}

void BuilderImpl::OnAttachWindow()
{
	m_editor->ComputeLayout();
}

void BuilderImpl::OnUpdate()
{
	GetFilesByGroup();

	Key32 domain_id;

	if (auto token = FindToken(m_content.a))
	{
		m_content.b = token->object;

		//refresh editor in case of external edit
		
		auto & resolved_path = token->attributes.resolved_path;

		domain_id = resolved_path.GetData()[0] == L':' ? Key32(Mid<true>(resolved_path, 1, Search(resolved_path, L':').value - 1)) : kNullKey;

		WString display = File::SplitFilename(resolved_path).b;

		if (m_supported_types.Search(m_content.a.type_id))
		{
			auto & state = GetState(token->address.id);

			m_editor->Store();

			if (auto & ref = state.a)
			{
				m_editor->SetContent(m_content.a.id, ref);
			}
			else
			{
				m_editor->SetContent(m_content.a.id, token->object);
			}

			auto parchive = &state.b;

			REFLEX_LOOP(idx, 2)
			{
				auto & archive = parchive[idx];

				if (archive) m_editor->Deserialize(True(idx), archive);
			}
		}
		else
		{
			m_editor->ClearContent();
		}

		GLX::SetText(m_source_selector, display);
	}
	else if (auto files = GetFilesOrdered())
	{
		if (Open(files.GetFirst(), true))
		{
			OnUpdate();

			return;
		}
	}

	GLX::Activate(m_save, m_islocal);

	bool editable = True(m_writeable_domains.Search(domain_id));

	GLX::Activate(m_edit, m_islocal && editable);

	m_editor->Activate(editable);

	if (GetError(m_content.b))
	{
		m_editor->ShowError(m_content.b);
	}
	else
	{
		m_editor->ClearError();
	}
}

void BuilderImpl::OnClock(Float32)
{
	if (m_monitor.Poll())
	{
		m_files.Clear();

		Update();
	}

	if (SetFiltered(m_editor->m_needs_commit, false))
	{
		m_editor->Store();

		Update();
	}
}

BuilderImpl::TextEditor::TextEditor(TRef <BuilderImpl> builder)
	: Interface(builder),
	m_texteditor(m_scroller.GetContent()->texteditor)
{
	m_scroller.SetStyle(builder->m_ide_styles["TextEditor"]);

	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::AddInlineFlex(*this, m_scroller);

	Update();

	m_texteditor->SetProperty(K32("ViewPort"), REFLEX_CREATE(GLX::Detail::LegacyWeakReferenceObject, m_scroller));

	m_scroller.InvertScrollAxis(true);
}

void BuilderImpl::TextEditor::OnSetContent(const Reflex::Object & data)
{
	auto view = m_scroller.GetContent();

	auto blob = Cast<Data::ArchiveObject>(data);

	m_texteditor->SetData(blob, GetPreference(kTabSpaces));

	view->ShowLineNumbers(GetPreference(kLineNumbers));
}

Reflex::Data::Archive BuilderImpl::TextEditor::GetBlob()
{
	return m_texteditor->GetData()->value;
}

void BuilderImpl::TextEditor::ClearError()
{
	GLX::Discard(*this, Data::kError);

	m_texteditor->ClearError();
}

void BuilderImpl::TextEditor::ShowError(const Reflex::Object & error)
{
	auto [line,stage,desc] = Data::GetError(error);

	if (line != kMaxUInt32) m_texteditor->SetError(line);

	auto error_display = AcquireFooter(Data::kError, L"Error");

	GLX::SetText(*error_display->GetFirst(), ToWString(stage));

	GLX::SetText(*error_display->GetLast(), ToWString(desc));
}

bool BuilderImpl::TextEditor::OnEvent(GLX::Object & src, GLX::Event & e)
{
	static constexpr auto DoSearch = [](TextEditor & self, bool reverse)
	{
		TRef prefs = TheGlobal::Get()->m_prefs;

		auto bits = MakeBits(reverse, GetPreference(kSearchOptionCaseSensitive), GetPreference(kSearchOptionWholeWord));

		auto fn = TextSearchBinder::Bind(bits);


		auto texteditor = self.m_texteditor;

		// Search in the editor's displayed buffer so match offsets stay aligned
		// with caret/reveal positions even when tabs are expanded or collapsed.
		WString text = GLX::GetText(*texteditor);

		auto needle = Data::GetWString(self, kSearch);

		auto caret_pos = texteditor->behaviour->GetCaret().a;

		if (caret_pos != Data::GetUInt32(self, kSearchCaret))
		{
			Data::SetUInt32(self, kSearchCaret, caret_pos);
		}

		REFLEX_LOOP(x, 2)
		{
			auto start = Data::GetUInt32(self, kSearch, kMaxUInt32);

			if (auto idx = fn(text, needle, start + 1))
			{
				texteditor->Reveal(idx.value, needle.size);

				Data::SetUInt32(self, kSearchCaret, idx.value);

				Data::SetUInt32(self, kSearch, idx.value);

				break;
			}
			else if (reverse && text.GetSize())
			{
				Data::SetUInt32(self, kSearch, text.GetSize() - 1);
			}
			else
			{
				Data::UnsetUInt32(self, kSearch);
			}
		}
	};

	static constexpr auto EndSearch = [](TextEditor & self)
	{
		GLX::Discard(self, kSearch);

		self.m_scroller.GetContent()->Focus();
	};

	auto texteditor = m_texteditor;

	if (e.id == Detail::TextEditor::kEdit)
	{
		CommitState(texteditor->GetData());

		return true;
	}
	else if (GLX::IsRightClick(e))
	{
		GLX::OpenContextMenu(m_scroller);

		return true;
	}
	else if (auto menu = GLX::GetMenu(e))
	{
		auto view = m_scroller.GetContent();

		auto line_numbers = GetPreference(kLineNumbers);

		GLX::BindClick(GLX::AddMenuOption(menu, L"Show Line Numbers", line_numbers), [view, line_numbers]()
		{
			SetPreference(kLineNumbers, !line_numbers);

			view->ShowLineNumbers(!line_numbers);
		});

		menu->AddSeparator();

		auto set_tab_spaces = [](TextEditorWithLineNumbers & view, UInt8 tab_spaces)
		{
			SetPreference(kTabSpaces, tab_spaces);

			auto current = AutoRelease(view.texteditor.GetData());

			view.texteditor.SetData(Null<Data::ArchiveObject>(), tab_spaces);

			view.texteditor.SetData(current, tab_spaces);
		};

		auto tab_spaces = GetPreference(kTabSpaces);

		GLX::BindClick(GLX::AddMenuOption(menu, L"Tabs", tab_spaces == 0), Bind(set_tab_spaces, view, UInt8(0)));
		GLX::BindClick(GLX::AddMenuOption(menu, L"3 spaces", tab_spaces == 3), Bind(set_tab_spaces, view, UInt8(3)));
		GLX::BindClick(GLX::AddMenuOption(menu, L"4 spaces", tab_spaces == 4), Bind(set_tab_spaces, view, UInt8(4)));

		return true;
	}
	else if (e.id == GLX::kKeyDown)
	{
		bool shift = false;

		switch (GLX_KEY_CODE(GLX::GetKeyCode(e), GLX::GetModifierKeys(e)))
		{
		case GLX_KEY_CODE(GLX::kKeyCodeF, GLX::kModifierKeyPrimary):
			{
				auto item = AcquireFooter(kSearch, L"Search", [this](GLX::Object & bar)
				{
					auto style = bar.GetStyle();

					auto textarea = GLX::AddInlineFlex(bar, GLX::Init(New<GLX::TextArea>(false), style["Search"]));

					auto option = style["Option"];

					TRef prefs = TheGlobal::Get()->m_prefs;

					const Pair <WString::View, Pair<Key32,bool>> options[] = { { L"Case", kSearchOptionCaseSensitive }, { L"Word", kSearchOptionWholeWord } };

					for (auto & i : options)
					{
						auto button = GLX::AddInline(bar, GLX::Init(New<GLX::Button>(i.a), option));

						GLX::BindClick(button, [prefs, option = i.b, button]()
						{
							auto value = !GetPreference(option);

							SetPreference(option, value);

							GLX::Select(button, value);
						});

						GLX::Select(button, GetPreference(i.b));
					}

					auto textedit = textarea->GetContent();

					GLX::SetText(textedit, Data::GetWString(*this, kSearch));

					GLX::SetEventDelegate(textedit, kZeroKey, [this, &bar](GLX::Object & src, GLX::Event & e)
					{
						if (e.id == GLX::kComplete)
						{
							Data::SetWString(*this, kSearch, GLX::GetText(src));

							DoSearch(*this, false);

							return true;
						}
						else if (e.id == GLX::kKeyDown)
						{
							auto keycode = GLX::GetKeyCode(e);

							if (keycode == GLX::kKeyCodeF3)
							{
								Data::SetWString(*this, kSearch, GLX::GetText(src));
							}
							else if (keycode == GLX::kKeyCodeEscape)
							{
								EndSearch(*this);

								return true;
							}
						}
						else if (e.id == GLX::kLoseFocus)
						{
							if (GLX::BranchContains(bar.GetParent(), GLX::Core::desktop->GetFocus()))
							{
								bar.GetFirst()->Focus();
							}
							else
							{
								EndSearch(*this);
							}

							return true;
						}

						return false;
					});
				});

				Data::UnsetUInt32(*this, kSearchCaret);

				auto textedit = Cast<GLX::TextArea>(item->value.GetFirst());

				item->value.GetFirst()->Focus();

				auto string = textedit->GetText();

				textedit->behaviour->SetCaret(string.size, 0, kMaxUInt32);
			}
			return true;

		case GLX_KEY_CODE(GLX::kKeyCodeF3, GLX::kModifierKeyShift):
			shift = true;
		case GLX_KEY_CODE(GLX::kKeyCodeF3, GLX::kModifierKeyNone):
			if (auto needle = Data::GetWString(*this, kSearch))
			{
				DoSearch(*this, shift);
			}
			else
			{
				EndSearch(*this);
			}
			return true;

		case GLX_KEY_CODE(GLX::kKeyCodeEscape, GLX::kModifierKeyNone):
			EndSearch(*this);
			return true;
		}
	}

	return Interface::OnEvent(src, e);
}

//BuilderImpl::PropertyEditor::PropertyEditor(BuilderImpl & builder)
//	: builder(builder),
//	listener(REFLEX_CREATE(Data::History::Listener, Bind(&PropertyEditor::OnNotification, this))),
//	onfocus(REFLEX_CREATE(GLX::Core::ObjectListener, Bind(&PropertyEditor::OnFocus, this, _P1))),
//	editor(IDE::Detail::PropertyEditor::Create())
//{
//	auto & ide_styles = builder.ide_styles;
//
//	auto & button = ide_styles["Button"];
//
//	reset.SetStyle(button);
//
//	open.SetStyle(button);
//
//	save.SetStyle(button);
//
//	track.SetStyle(button);
//
//	refresh.SetStyle(button);
//
//	footer.SetStyle(ide_styles["Bar"]);
//
//	GLX::SetText(reset, L"Reset");
//
//	GLX::SetText(open, L"Open");
//
//	GLX::SetText(save, L"Save");
//
//	GLX::SetText(track, L"Track");
//
//	GLX::SetText(refresh, L"Refresh");
//
//
//
//	SetFlow(GLX::kFlowY);
//
//	GLX::AddInlineFlex(*this, editor);
//
//	GLX::AddInline(footer, refresh);
//
//	GLX::AddFloat(footer, track, GLX::kRight);
//
//	GLX::AddInline(*this, footer);
//
//	listener->Attach(editor);
//}
//
//void BuilderImpl::PropertyEditor::OnNotification()
//{
//	if (auto archive = Data::EncodePropertySet(File::kPropertySheetFormat, editor.GetData()))
//	{
//		//Detail::StoreEditedResource(builder.m_path, archive);
//
//		builder.RequestReload();
//
//		Update();
//	}
//}
//
//void BuilderImpl::PropertyEditor::OnFocus(GLX::Object & object)
//{
//	REFLEX_INLINE_LOCAL(Data::PropertySet*, GetChildByID)(Data::PropertySet & parent, Key32 id)
//	{
//		for (auto & i : parent.Iterate<Data::PropertySet>())
//		{
//			if (i.key.id == id) return RemoveConst(i.value.Adr());
//		}
//
//		return 0;
//	}
//	REFLEX_END;
//
//	auto & window = builder.GetWindow()->GetContent();
//
//	if (IsValid(object) && !BranchContains<GLX::Object>(window, object))
//	{
//		auto & style = RemoveConst(object.GetAppliedStyle());
//
//		auto sheet = GLX::Detail::GetObjectByClass<GLX::StyleSheet>(GLX::Style::ParentRange(style));
//
//		if (sheet && sheet != &builder.ide_styles)
//		{
//			builder.Open(sheet->GetPath(), false);
//
//			Array <Key32> path;
//
//			for (auto & i : GLX::Style::ParentRange(style))
//			{
//				auto key = i.GetID();
//
//				path.Push(key);
//			}
//
//			path.Pop();
//
//			if (path)
//			{
//				auto & root = editor.GetData();
//
//				auto itr = &root;
//
//				while (auto child = GetChildByID::Call(*itr, path.GetLast()))
//				{
//					itr = child;
//
//					path.Pop();
//
//					if (!path) break;
//				}
//
//				REFLEX_ASSERT(editor.SetFocus(*itr));
//			}
//		}
//	}
//}
//
//bool BuilderImpl::PropertyEditor::OnEvent(GLX::Object & src, GLX::Event & e)
//{
//	if (e.id == kMouseDown)
//	{
//		if (src == reset)
//		{
//			//Detail::DiscardEditedResource(builder.m_path);
//
//			builder.RequestReload();
//
//			Update();
//
//			return true;
//		}
//		else if (src == open)
//		{
//			//Detail::OpenLocalResource(builder.m_path);
//
//			Update();
//
//			return true;
//		}
//		else if (src == save)
//		{
//			Detail::SaveResource(builder.m_path, GetContent());
//
//			builder.RequestReload();
//
//			Update();
//
//			return true;
//		}
//		else if (src == track)
//		{
//			GLX::Select(track, !GLX::Selected(track));
//
//			Update();
//
//			return true;
//		}
//		else if (src == refresh)
//		{
//			GLX::Restart();
//
//			return true;
//		}
//	}
//
//	return false;
//}
//
//void BuilderImpl::PropertyEditor::PopulateClientArea(GLX::Object & object)
//{
//	GLX::AddInline(object, reset);
//
//	GLX::AddInline(object, open);
//
//	GLX::AddInline(object, save);
//
//	//auto & state = panel.m_state;
//
//	//File::RestorePropertyUNUSED(state, K32("editor"), editor);
//
//	//track.Select(File::UnpackPropertyUNUSED(state, K32("track"), false));
//}
//
//void BuilderImpl::PropertyEditor::SetContent(const WString & path, Data::Archive && data)
//{
//	auto archive = Data::EncodePropertySet(File::kPropertySheetFormat, editor.GetData());
//
//	if (IsNull(editor.GetData()) || archive != data)
//	{
//		auto properties = New<Data::PropertySet>();
//
//		Data::DecodePropertySet(File::kPropertySheetFormat, data, properties);
//
//		editor.SetData(Detail::PropertyEditor::CreatePropertySheetInterface(properties), properties);
//	}
//
//	Update();
//}
//
//Reflex::Data::Archive BuilderImpl::PropertyEditor::GetContent()
//{
//	return Data::EncodePropertySet(File::kPropertySheetFormat, editor.GetData());
//}
//
//void BuilderImpl::PropertyEditor::Serialize(Data::Archive & stream)
//{
//	stream(editor);
//
//	stream(True(onfocus->GetList()));
//}
//
//void BuilderImpl::PropertyEditor::Deserialize(Data::Archive::View & stream)
//{
//	stream(editor);
//
//	bool focus;
//
//	stream(focus);
//
//	GLX::Select(track, focus);
//}
//
//void BuilderImpl::PropertyEditor::OnUpdate()
//{
//	if (GLX::Selected(track))
//	{
//		GLX::Core::desktop.ConnectOnFocus(onfocus);
//	}
//	else
//	{
//		onfocus->Detach();
//	}
//
//	auto & path = builder.m_path;
//
//	//bool edited = Detail::HasEditedResource(path);
//
//	//GLX::Activate(reset, Or(edited, Detail::HasLocalResource(path)));
//
//	//bool saveable = Or(edited, !System::Exists(Detail::GetResourceLocalPath(path)));
//
//	//GLX::Activate(save, saveable);
//}

Detail::ConsolePanel::Ctr gBuilderCtr(L"Builder", -3, []() -> TRef <Detail::ConsolePanel>
{
	return REFLEX_CREATE(BuilderImpl);
});

REFLEX_END_INTERNAL

//Reflex::IDE::Detail::ConsolePanel::Ctr Reflex::IDE::gBuilderCtr(L"Builder", -3, []() -> Reflex::TRef <Reflex::IDE::Detail::ConsolePanel>
//{
//	return REFLEX_CREATE(BuilderImpl);
//});
