#include "stylesheet.h"
#include "../library.h"
#include "detail/layer.h"

#include "reflex/glx/detail/resource.h"
#include "reflex/glx/detail/vector.h"
#include "reflex/glx/detail/svg.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_INLINE ConstTRef <Style> ParseAncestor(const Style & self, Key32 id, Address address, Reflex::Object & object)
{
	if (address.id == id)
	{
		auto ref = AutoRelease(object);

		if (address.type_id == REFLEX_TYPEID(Data::Key32Property))
		{
			return Detail::FindStyle(self, ToView(Cast<Data::Key32Property>(object)->value));
		}
		else if (address.type_id == REFLEX_TYPEID(Data::ArrayOfKey32Property))
		{
			return Detail::FindStyle(self, Cast<Data::ArrayOfKey32Property>(object)->value);
		}
	}

	return Style::null;
}

struct AliasStyle : public StyleAccessor
{
	REFLEX_OBJECT(AliasStyle, Style);

	AliasStyle(Key32 id)
		: StyleAccessor(id, 0)
	{
		Style::m_non_virtual = false;
	}


	const Style * QuerySubStyle(Key32 id, const Style * fallback) const override;

	void OnSetProperty(Address address, Reflex::Object & object) override;

	void OnQueryProperty(Address address, Reflex::Object * & pobject) const override;


	void SetTarget(ConstTRef <Style> source)
	{
		REFLEX_ASSERT(IsValid(source) && source.Adr() != this);

		if (!Cast<StyleAccessor>(source)->m_non_virtual)
		{
			source = Cast<AliasStyle>(source)->m_target.Load();

			REFLEX_ASSERT(Cast<StyleAccessor>(source)->m_non_virtual);
		}

		m_target.Store(source);

		RemoveConst(stylesheet_flags) = source->stylesheet_flags;

		m_is_root_style = Cast<StyleAccessor>(source)->m_is_root_style;

		MemCopy(Cast<StyleAccessor>(source)->m_states, m_states, sizeof(m_states));
	}


	Reflex::Detail::ConstWeakRef <Style> m_target;
};

void PopulateStates(StyleAccessor & style)
{
	auto pstate = style.m_states;

	auto pend = pstate + GetArraySize(style.m_states);

	for (auto & i : style)
	{
		if (!Cast<StyleAccessor>(i)->m_is_root_style)
		{
			*pstate++ = i.id;

			if (pstate == pend) return;
		}
	}

	while (pstate != pend) *pstate++ = kNullKey;
}

void Inherit(const Style & source, StyleAccessor & dest)
{
	if (dest.m_non_virtual)
	{
		//properties

		for (auto & i : RemoveConst(source).Iterate())
		{
			switch (i.key.id.value)
			{
			case Detail::kComputedStyle.value:
			case Reflex::Detail::AbstractWeakRef::kWeakReferences:
				break;

			default:
				dest.SetProperty(i.key, i.value);
				break;
			}
		}


		//children

		for (auto & child : source)
		{
			auto id = child.id;

			if (auto existing = RemoveConst(Detail::GetChildStyle(dest, id)))
			{
				Inherit(child, *Cast<StyleAccessor>(existing));
			}
			else
			{
				TRef <StyleAccessor> clone = kNoValue;

				auto stylesheet_flags = child.stylesheet_flags;

				if (Cast<StyleAccessor>(child)->m_non_virtual)
				{
					clone = Cast<StyleAccessor>(Style::Create(child.id, stylesheet_flags));

					Inherit(child, *clone);

					clone->m_is_root_style = Cast<StyleAccessor>(child)->m_is_root_style;
				}
				else
				{
					auto alias = New<AliasStyle>(id);

					alias->SetTarget(Cast<AliasStyle>(child)->m_target.Load());

					clone = alias;
				}

				clone->Attach(dest);
			}
		}

		PopulateStates(dest);
	}
	else
	{
		ThrowStylesheetParseError({}, "can not modify Alias");
	}
}

template <bool ALIAS> REFLEX_INLINE const Style * QueryChildImpl(const StyleAccessor * self, const StyleAccessor * alias, Key32 id, const Style * fallback)
{
	auto fetch = Detail::GetChildStyle(*(ALIAS ? alias : self), id, fallback);

	if (self->m_is_root_style) return fetch;

	if (fetch == fallback)	//nothing found
	{
		auto itr = Cast<StyleAccessor>(self->GetParent());

		bool was_not_root = true;

		while (itr && was_not_root)
		{
			if (auto t = itr->QuerySubStyle(id)) return t;

			was_not_root = !itr->m_is_root_style;

			itr = Cast<StyleAccessor>(itr->GetParent());
		}
	}

	return fetch;
}

template <bool ALIAS> REFLEX_INLINE void QueryPropertyImpl(const StyleAccessor * self, const StyleAccessor * alias, Address address, Reflex::Object * & pobject)
{
	Reflex::Object * fetch = pobject;

	if constexpr (ALIAS)
	{
		if (address.id == Detail::kComputedStyle)
		{
			self->QueryProperty<false>(address, fetch);
		}
		else
		{
			alias->QueryProperty<false>(address, fetch);
		}
	}
	else
	{
		self->QueryProperty<false>(address, fetch);
	}

	if (self->m_is_root_style || address.id == Detail::kComputedStyle)
	{
		//pobject = fetch;

		//return;
	}
	else if (fetch == pobject)	//nothing found
	{
		auto itr = Cast<StyleAccessor>(self->GetParent());

		bool was_not_root = true;

		while (itr && was_not_root)
		{
			if (itr->QueryProperty<true>(address, pobject)) return;

			was_not_root = !itr->m_is_root_style;

			itr = Cast<StyleAccessor>(itr->GetParent());
		}
	}

	pobject = fetch;
}

void AliasStyle::OnSetProperty(Address address, Reflex::Object & object)
{
	if (auto ancestor = ParseAncestor(*this, K32("source"), address, object))
	{
		SetTarget(ancestor);

		PopulateStates(*Cast<StyleAccessor>(GetParent()));
	}
	else if (address == MakeAddress<Data::CStringProperty>("path"))
	{
		auto path = Cast<Data::CStringProperty>(object);

		SetTarget(g_current_stylesheet.sheet->AddInclude(path->value, true));

		AutoRelease(object);
	}
	else
	{
		Style::OnSetProperty(address, object);
	}
}

const Style * AliasStyle::QuerySubStyle(Key32 id, const Style * fallback) const
{
	return QueryChildImpl<true>(this, Cast<StyleAccessor>(m_target.Load().Adr()), id, fallback);
}

void AliasStyle::OnQueryProperty(Address address, Reflex::Object * & pobject) const
{
	QueryPropertyImpl<true>(this, Cast<StyleAccessor>(m_target.Load().Adr()), address, pobject);
}

REFLEX_END_INTERNAL

Reflex::GLX::Style::Style(Key32 id, UInt16 stylesheet_flags)
	: id(id)
	, stylesheet_flags(stylesheet_flags)
{
}

void Reflex::GLX::Style::OnSetProperty(Address address, Reflex::Object & object)
{
	if (auto child = DynamicCast<Style>(object))	//PropertySheet parser automatically attaches objects after brace close, but we already attached it in the StyleSheetParser::CreateObject override
	{
		REFLEX_ASSERT(child->GetParent() == this);

		REFLEX_ASSERT(Detail::GetChildStyle(*this, address.id) == child);
	}
	else if (address.type_id == g_layer_desc_typeid)
	{
		auto descs = New<Detail::ArrayOfLayerDesc>();

		descs->value.Push(Cast<Detail::LayerDesc>(object));

		address.type_id = g_array_of_layer_desc_typeid;

		SetProperty(address, descs);
	}
	else if (address.type_id == g_resource_desc_typeid)
	{
		auto param = AutoRelease(Cast<Detail::ResourceDesc>(object));

		const Reflex::Object * obj;

		switch (param->type.value)
		{
		case K32("Font"):
			address.type_id = g_font_typeid;
			obj = Detail::Font::Create(param).Adr();
			break;

		case K32("Bitmap"):
			address.type_id = g_imageset_typeid;
			obj = Detail::CreateImageSet(param).Adr();
			break;

		case K32("SVG"):
			address.type_id = g_imageset_typeid;
			obj = Detail::CreateImageSetFromSVG(param).Adr();
			break;

		case K32("VectorSet"):
			address.type_id = g_imageset_typeid;
			obj = Detail::CreateLegacyVectorSet(param).Adr();
			break;

		default:
			return;
		}

		SetProperty(address, *RemoveConst(obj));
	}
	else if (auto ancestor = ParseAncestor(*this, K32("inherit"), address, object))
	{
		Inherit(Cast<StyleAccessor>(ancestor), *Cast<StyleAccessor>(this));
	}
#if REFLEX_DEBUG
	else if (address.type_id == GetTypeID<Data::ArrayOfCStringProperty>())
	{
		auto layers = Detail::ConvertLegacyLayers(Cast<Data::ArrayOfCStringProperty>(object));

		Data::PropertySet::OnSetProperty(MakeAddress<Detail::ArrayOfLayerDesc>(address.id), layers);
	}
	else if (address.type_id == GetTypeID<Detail::ReferenceProperty>())
	{
		ThrowInvalidPropertyError(object_t->tname, address.id, object, false);

		AutoRelease(object);
	}
#endif
	else
	{
		Data::PropertySet::OnSetProperty(address, object);
	}
}

void Reflex::GLX::Style::OnReleaseData()
{
	//TODO properly resolve circulars, without removing needed style data
	//this causing stylehsheet to be messed up when vm context is destroyed

	Data::UnsetAll<Data::PropertySet>(*this);

	Data::UnsetAll<GLX::Object>(*this);
}

Reflex::TRef <Reflex::GLX::Style> Reflex::GLX::Style::AddSubStyle(Key32 type, Key32 id, bool is_stub)
{
	REFLEX_ASSERT(!Detail::GetChildStyle(*this, id));	//parser takes care of reusing existing

	constexpr auto AddState = [](TRef <Style> style, Key32 id)
	{
		for (auto & i : style->m_states)
		{
			if (i.value == kHashSeed)
			{
				i = id;

				return;
			}
		}

		ThrowStylesheetParseError({}, "too many States");
	};

	TRef <Style> child = kNoValue;

	switch (type.value)
	{
	case K32("State"):
		child = Style::Create(id, stylesheet_flags);
		child->m_is_root_style = false;
		AddState(this, id);
		break;

	case K32("Alias"):
		child = New<AliasStyle>(id);
		if (is_stub)
		{
			Cast<AliasStyle>(child)->SetTarget(Detail::FindStyle(*this, { id }));

			if (!child->m_is_root_style) AddState(this, id);
		}
		break;

	default:
		child = Style::Create(id, stylesheet_flags);
		child->m_is_root_style = true;
		break;
	}

	child->Attach(*this);

	return child;
}

Reflex::TRef <Reflex::GLX::Style> Reflex::GLX::Style::Create(Key32 id, UInt16 stylesheet_flags)
{
	return REFLEX_CREATE(Style, id, stylesheet_flags);
}

const Reflex::GLX::Style * Reflex::GLX::Style::QuerySubStyle(Key32 id, const Style * fallback) const
{
	return QueryChildImpl<false>(Cast<StyleAccessor>(this), 0, id, fallback);
}

void Reflex::GLX::Style::OnQueryProperty(Address address, Reflex::Object * & pobject) const
{
	QueryPropertyImpl<false>(Cast<StyleAccessor>(this), 0, address, pobject);
}

template <bool VIRTUAL> REFLEX_INLINE bool Reflex::GLX::StyleAccessor::QueryProperty(Address address, Reflex::Object * & in_out) const
{
	auto test = in_out;

	if constexpr (VIRTUAL)
	{
		RemoveConst(this)->OnQueryProperty(address, in_out);
	}
	else
	{
		RemoveConst(this)->Data::PropertySet::OnQueryProperty(address, in_out);
	}

	return test != in_out;
}

#if REFLEX_DEBUG
void Reflex::GLX::ThrowStylesheetParseError(const CString::View & error_type, const CString::View & error_value)
{
	if (auto pline = Data::Detail::PropertySheetInterface::LineScope::GetCurrent())
	{
		if (error_type)
		{
			Data::Detail::TokeniseFail(*pline, Join(error_type, ':', ' ', error_value));
		}
		else
		{
			Data::Detail::TokeniseFail(*pline, error_value);
		}
	}
	else
	{
		output.Error(error_type, error_value);
	}
}

Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::Style::operator[](const char * id) const
{
	Data::RegisterKey(g_library->keymap, id);

	return this->operator[](Key32(ToView(id)));
}
#endif
