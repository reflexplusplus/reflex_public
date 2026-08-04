#include "../../../../include/reflex/glx/style/functions.h"




//
//impl

void Reflex::GLX::SetOnStyle(Object & object, const Function <void(const Style&)> & callback, Key32 delegate_id)
{
	struct Delegate : public GLX::Object::Delegate
	{
		Delegate(const Function <void(const Style & style)> & callback) : m_callback(callback) {}

		void OnAttachObject() override
		{
			if (auto current = object->GetStyle())
			{
				OnSetStyle(current);
			}
		}

		void OnSetStyle(const Style & style) override
		{
			m_callback(style);
		}

		const Function <void(const Style & style)> m_callback;
	};

	object.SetDelegate(delegate_id, New<Delegate>(callback));
}

Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::Detail::FindStyle(const Style & origin, const ArrayView <Key32> & path)
{
	static constexpr auto Validate = [](const Style & origin, const Style * ancestor)
	{
		if (ancestor == &origin) ThrowStylesheetParseError({}, "FindStyle failed");

		return ancestor;
	};

	constexpr auto LocateRoot = [](const Style & origin, Key32 id) -> const Style *
	{
		auto itr = &origin;

		auto root = itr;

		while (itr)
		{
			auto prev = itr->GetPrev();

			while (prev)
			{
				if (prev->id == id) return prev;

				prev = prev->GetPrev();
			}

			root = itr;

			itr = itr->GetParent();
		}

		if (auto sheet = DynamicCast<StyleSheet>(*root))
		{
			const Style * match = 0;

			Detail::EnumerateIncludes(*sheet, [id, &match](const StyleSheet & i)
			{
				if (i.id == id)
				{
					match = &i;

					return true;
				}
				else if (auto child = i.QuerySubStyle(id, nullptr))
				{
					match = child;

					return true;
				}

				return false;
			});

			return Validate(origin, match);
		}
		else
		{
			return 0;
		}
	};

	if (auto source = LocateRoot(origin, path.GetFirst()))
	{
		auto local = Splice(path, 1).b;

		for (auto & i : local)
		{
			source = Detail::GetChildStyle(*source, i, &Style::null);
		}

		return Validate(origin, source);
	}

	ThrowStylesheetParseError({}, "FindStyle failed");

	return Style::null;
}

const Reflex::Object * Reflex::GLX::Detail::FindResource(const Style & style, Address adr, const Reflex::Object * fallback)
{
	REFLEX_INLINE_LOCAL(const Reflex::Object*, Locate)(const Style & style, Address adr, const Style * &root)
	{
		const Style * pstyle = &style;

		while (pstyle)
		{
			//if (pstyle->kIsResourceOwner)
			{
				if (auto resource = RemoveConst(pstyle)->QueryProperty(adr))
				{
					return resource;
				}
			}

			root = pstyle;

			pstyle = pstyle->GetParent();
		}

		return 0;
	}
	REFLEX_END

	const Style * root = &Style::null;

	if (auto res = Locate::Call(style, adr, root)) return res;

	if (auto sheet = DynamicCast<StyleSheet>(*root))
	{
		//WAS _GetIncludes

		const Reflex::Object * res = 0;

		EnumerateIncludes(*sheet, [adr, &root, &res](const StyleSheet & i)
		{
			if (auto t = Locate::Call(i, adr, root))
			{
				res = t;

				return true;
			}

			return false;
		});

		if (res) return res;
	}

	return fallback;
}

bool Reflex::GLX::Detail::EnumerateIncludes(const StyleSheet & stylesheet, const Function <bool(const StyleSheet&)> & enumerator)
{
	REFLEX_LOCAL(bool,Recurse)(const StyleSheet & stylesheet, const Function <bool(const StyleSheet&)> & enumerator)
	{
		if (enumerator(stylesheet))
		{
			return true;
		}
		else
		{
			for (auto & i : stylesheet.GetIncludes(false))
			{
				if (auto p = Call(i, enumerator)) return true;
			}

			return false;
		}
	}
	REFLEX_END

	return Recurse::Call(stylesheet, enumerator);
}

bool Reflex::GLX::Detail::ExtractProperty(const Style & style, Key32 id, Reflex::Object & out)
{
	auto type_id = out.object_t->type_id;

	auto numbers = Data::GetFloat32Array(style, id);

	if (type_id == REFLEX_TYPEID(MarginProperty))
	{
		Cast<MarginProperty>(out)->value = ToMargin(numbers, style.stylesheet_flags);
	}
	else if (type_id == REFLEX_TYPEID(SizeProperty))
	{
		Cast<SizeProperty>(out)->value = ToSize(numbers);
	}
	else if (type_id == REFLEX_TYPEID(ColourProperty))
	{
		Cast<ColorProperty>(out)->value = ToColour(numbers);
	}
	else
	{
		REFLEX_ASSERT("unsupported");
		
		return false;
	}

	return True(numbers);
}

Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::FindStyle(const Object & object, Key32 id)
{
	TRef itr = object;
	
	while (itr)
	{
		if (auto valid = itr->GetStyle())
		{
			return FindStyle(valid, id);
		}

		itr = itr->GetParent();
	}

	return {};
}
