#include "layout.h"




//TODO derive from iresource, not compose (but not critical, only for VM anyway)

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct Layout : public Reflex::Data::Detail::PropertySheetInterface
{
	typedef Data::PropertySheet::String String;
	typedef Data::PropertySheet::Attribute Attribute;
	typedef Data::PropertySheet::ArrayOfAttributes Attributes;

	typedef Data::PropertySheet::ObjectArray <GLX::Style> Styles;

	inline static const Pair <Key32,UInt8> kFlowIDs[] =
	{
		{ K32("y"), kFlowY },
		{ K32("invert"), kFlowInvert },
		{ K32("center"), kFlowCenter },
		//{ K32("wrap"), kFlowWrap },
	};

	inline static const Pair <Key32, Tuple< Positioning, Orientation, Orientation> > kAlignIDs[] =
	{
		{ K32("inline"), { kPositioningInline, kOrientationNear, kOrientationFit } },
		{ K32("float"), kPositioningFloat, kOrientationFit, kOrientationFit },
		{ K32("absolute"), kPositioningAbsolute, kOrientationNear, kOrientationNear },

		{ K32("inline-flex"), { kPositioningInline, kOrientationFit, kOrientationFit } },

		{ K32("top_left"), { kPositioningFloat, kOrientationNear, kOrientationNear } },
		{ K32("top"), { kPositioningFloat, kOrientationCenter, kOrientationNear } },
		{ K32("top_right"), { kPositioningFloat, kOrientationFar, kOrientationNear } },
		{ K32("left"), { kPositioningFloat, kOrientationNear, kOrientationCenter } },
		{ K32("center"), { kPositioningFloat, kOrientationCenter, kOrientationCenter } },
		{ K32("right"), { kPositioningFloat, kOrientationFar, kOrientationCenter } },
		{ K32("bottom_left"), { kPositioningFloat, kOrientationNear, kOrientationFar } },
		{ K32("bottom"), { kPositioningFloat, kOrientationCenter, kOrientationFar } },
		{ K32("bottom_right"), { kPositioningFloat, kOrientationFar, kOrientationFar } },
	};

	inline static const Pair <Key32,Orientation> kPositionIDs[] =
	{
		{ K32("fit"), kOrientationFit },
		{ K32("auto"), kOrientationNear },//for inline
		{ K32("near"), kOrientationNear },
		{ K32("center"), kOrientationCenter },
		{ K32("far"), kOrientationFar },
	};

	static inline UInt8 ApplyFlow(const ArrayView <Key32> & ids)
	{
		const Pair <Key32, UInt8> null;

		UInt8 flags = 0;

		for (auto & i : ids)
		{
			flags |= UInt8(SearchValue<KeyCompare>(ToView(kFlowIDs), i, &null)->b);
		}

		return flags;
	}

	static inline void ApplyAlign(GLX::Object & object, Key32 id)
	{
		auto align = SearchValue<KeyCompare>(ToView(kAlignIDs), id, 0, kAlignIDs)->b;

		GLX::Detail::SetPositioning(object, align.a, align.b, align.c);
	}

	static inline void ApplyPosition(GLX::Object & object, const ArrayView <Key32> & ids)
	{
		auto alignflags = object.GetPositioningFlags();

		const Pair <Key32, Orientation> null;

		Positioning positioning = Positioning(alignflags & 3);

		Orientation alignment[] = { Orientation((alignflags >> 2) & 3), Orientation((alignflags >> 4) & 3) };

		UInt8 x = 0;

		for (auto & i : ids)
		{
			alignment[x++ & 1] = SearchValue<KeyCompare>(ToView(kPositionIDs), i, 0, &null)->b;
		}

		Detail::SetPositioning(object, positioning, alignment[0], alignment[1]);
	}

	template <class TYPE> static inline TYPE * CastProperty(ObjectWithType & o)
	{
		if (o.b == REFLEX_TYPEID(TYPE)) return Cast<TYPE>(o.a.Adr());

		return 0;
	}

	static void ApplyStyle(GLX::Object & object)
	{
		auto FindRoot = [](GLX::Object & object, Key32 id) -> const Style &
		{
			auto pparent = &object;

			while (IsValid(*pparent))
			{
				if (auto styles = QueryProperty<Styles>(*pparent, kNullKey))
				{
					for (auto & i : *styles)
					{
						if (i->id == id)
						{
							return i;
						}
					}
				}

				pparent = pparent->GetParent().Adr();
			}

			return REFLEX_NULL(Style);
		};

		if (auto id = QueryProperty<Attribute>(object, K32("style")))
		{
			object.SetStyle(FindRoot(object, id->value));
		}
		else if (auto ids = QueryProperty<Attributes>(object, K32("style")))
		{
			if (*ids)
			{
				auto pstyle = &FindRoot(object, ids->GetFirst());

				REFLEX_FOREACH(id, Splice(*ids, 1).b) pstyle = (*pstyle)[id].Adr();

				object.SetStyle(*pstyle);
			}
		}
	}

	Layout()
	{
	}

	virtual bool Begin(Data::PropertySet & dynamic) const override
	{
		return DynamicCast<GLX::Object>(dynamic);
	}

	virtual void End(Data::PropertySet & dynamic) const override
	{
		//TODO THis is not needed now, lookup in place

		REFLEX_LOCAL(void, Recurse)(GLX::Object & object)
		{
			ApplyStyle(object);

			for (auto & i : object)
			{
				//ApplyStyle(i);

				Call(i);
			}
		}
		REFLEX_END

		//ApplyStyle(Cast<GLX::Object>(dynamic));

		Recurse::Call(Cast<GLX::Object>(dynamic));
	}

	virtual PropertySheetInterface & GetInterface(ObjectWithType & object) override
	{
		if (DynamicCast<GLX::Object>(object.a))
		{
			return *this;
		}
		else if (object.b == REFLEX_TYPEID(Style))
		{
			return GLX::Detail::istylesheet;
		}
		else
		{
			return GLX::Detail::iresource;
		}
	}

	virtual ObjectWithType CreateObject(Reflex::Object & parent, const CString::View & type, Key32 id) const override
	{
		switch (Key32(type).GetValue())
		{
		case K32("Style"):
			return Data::Detail::MakeObjectWithType(*REFLEX_CREATE(GLX::Style, 1.0f, id));

		case K32("State"):
			return Data::Detail::MakeObjectWithType(*GLX::Style::CreateState(1.0f, id));

		//case K32("Font"):
		//case K32("Bitmap"):
		//case K32("VectorSet"):
		//	return GLX::Detail::istylesheet.CreateObject()

		case K32("Text"):
			return Data::Detail::CreateObjectWithType<Detail::PreCompiledResource>(K32("Text"));

		default:
			return Data::Detail::MakeObjectWithType<GLX::Object>(GLXVM::CreateObject(*m_context, type));
		}
	}

	virtual ObjectWithType CreateObjectArray(const CString::View & type, const Array <ObjectWithType> & values) const override { return {}; }

	virtual ObjectWithType CreateValue(Data::KeyMap & keymap, const CString::View & type, TokenType tokentype, const CString::View & value) const override
	{
		return GLX::Detail::iresource.CreateValue(keymap, type, tokentype, value);
	}

	virtual ObjectWithType CreateValueArray(Data::KeyMap & keymap, const CString::View & type, TokenType tokentype, const Array <CString::View> & values) const override
	{
		return GLX::Detail::iresource.CreateValueArray(keymap, type, tokentype, values);
	}

//	virtual void Store(Data::KeyMap & keymap, Stack stack, TokenType key_t, const CString::View & key, ObjectWithType && value) const override
//	{
//		auto & dynamic = *stack.GetLast();
//
//		auto & object = Cast<GLX::Object>(dynamic);
//
//		switch (Key32(key).GetValue())
//		{
//		case K32("style"):
//			if (auto styleid = CastProperty<Attribute>(value))
//			{
//				SetProperty(dynamic, K32("style"), *styleid);
//			}
//			else if (auto styleids = CastProperty<Attributes>(value))
//			{
//				if (*styleids) SetProperty(dynamic, K32("style"), *styleids);
//			}
//			//else if (auto style = CastProperty<GLX::Style>(value))
//			//{
//			//	//if (auto object = DynamicCast<GLX::Object>(dynamic))
//			//	{
//			//		object.SetStyle(*style);
//			//	}
//			//}
//			break;
//
//		case K32("flow"):
//			if (auto flowids = CastProperty<Attributes>(value))
//			{
//				SetFlow(object, ApplyFlow(*flowids));
//			}
//			else if (auto flowid = CastProperty<Attribute>(value))
//			{
//				SetFlow(object, ApplyFlow({ *flowid }));
//			}
//			break;
//
//		case K32("align"):
//			if (auto id = CastProperty<Attribute>(value))
//			{
//				ApplyAlign(object, *id);
//			}
//			break;
//
//		case K32("position"):
//			if (auto ids = CastProperty<Attributes>(value))
//			{
//				ApplyPosition(object, *ids);
//			}
//			else if (auto id = CastProperty<Attribute>(value))
//			{
//				ApplyPosition(object, { *id });
//			}
//			break;
//
//		//case K32("clip"):
//		//	if (auto clipids = CastProperty<Attributes>(value))
//		//	{
//		//		if (clipids->GetSize())
//		//		{
//		//			auto x = clipids->GetFirst() == ktrue;
//		//			auto y = clipids->GetLast() == ktrue;
//
//		//			object.EnableClip(x, y);
//		//		}
//		//	}
//		//	else if (auto pclip = CastProperty<Attribute>(value))
//		//	{
//		//		auto clip = pclip->value == ktrue;
//
//		//		object.EnableClip(clip, clip);
//		//	}
//		//	break;
//
//		case K32("autofit"):
//			if (auto clipids = CastProperty<Attributes>(value))
//			{
//				if (clipids->GetSize())
//				{
//					auto x = clipids->GetFirst() == ktrue;
//					auto y = clipids->GetLast() == ktrue;
//
//					GLX::EnableAutoFit(object, x, y);
//				}
//			}
//			else if (auto pclip = CastProperty<Attribute>(value))
//			{
//				auto clip = pclip->value == ktrue;
//
//				GLX::EnableAutoFit(object, clip, clip);
//			}
//			break;
//
//		case GLX::kid:
//			if (auto id = CastProperty<Attribute>(value))
//			{
//				object.SetID(id->value);
//			}
//			break;
//
//		default:
//			if (value.b == REFLEX_TYPEID(GLX::Object))
//			{
//				auto & child = Cast<GLX::Object>(*value.a);
//
//				child.SetID(Key32(key));
//
//				child.SetParent(object);
//			}
//			else if (value.b == REFLEX_TYPEID(GLX::Style))
//			{
//				auto & style = Cast<Style>(*value.a);
//
//				if (key)
//				{
//					Data::Detail::AcquireProperty<Styles>(object, kNullKey)->Push(style);
//				}
//				else
//				{
//					object.SetStyle(style);
//				}
//			}
//			else if (auto ptext = CastProperty<Detail::PreCompiledResource>(value))
//			{
//				if (ptext->type == K32("Text"))
//				{
//					SetProperty(dynamic, key, REFLEX_CREATE(Text, ToWString(*GetProperty<String>(*ptext, GLX::kvalue))));
//				}
//			}
//			else
//			{
//				dynamic.SetObject({ key, value.b }, value.a);
//			}
//			break;
//		}
//	}

	VM::Context * m_context = 0;

	//const WString * m_path = 0;
};

Layout kLayoutFormat;

REFLEX_END_INTERNAL


VM_WRAP_INTERNAL(GLX)

Data::Detail::PropertySheetInterface & kLayout = GLX::kLayoutFormat;

REFLEX_END_INTERNAL

//Reflex::TRef <Reflex::GLX::Object> Reflex::GLXVM::CreateLayout(VM::Context & context, const WString & path)
//{
//	struct TrackingToken : public Reflex::Object
//	{
//		bool OnInvoke(Address address, void * ptr) override
//		{
//			if (auto geterror = CastCall<TRef<Data::PropertySet>>(address, ptr, K32("GetError")))
//			{
//				geterror->rtn = m_error;
//
//				return true;
//			}
//
//			return false;
//		}
//
//		Reference <Data::PropertySet> m_error;
//	};
//
//	auto resolved = ResolveIncludePath(context, path);
//
//	File::Attributes attributes = { .domain_id = File::kdisk };
//
//	System::GetFileAttributes(resolved, attributes.size_time);
//
//	File::ResourcePool::Lock lock(GLX::Core::desktop.resourcepool);
//
//	lock.InsertToken(resolved, attributes, REFLEX_TYPEID(TrackingToken), REFLEX_CREATE(TrackingToken));
//
//	auto object = New<Object>(context);
//
//	SilentReference retain(object);
//
//	GLX::kLayoutFormat.m_context = &context;
//
//	Data::DecodePropertySet(Reflex::The<Library>::Get()->format, File::Open(resolved), object);
//
//	return object;
//}
