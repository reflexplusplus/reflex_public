#include "string_format.h"
#include "../../../../include/reflex/data/functions/url.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

//CString CStringToString(const KeyMap & keymap, const Object & object)
//{
//	return EncodeUrlSegment(Cast<CStringProperty>(object)->value);
//}
//
//CString WStringToString(const KeyMap & keymap, const Object & object)
//{
//	auto utf8 = EncodeUTF8(Cast<ObjectOf<WString>>(object)->value);
//
//	return EncodeUrlSegment(Unpack<CString::View>(utf8));
//}
//
//CString Key32ToStringUrlEncoded(const KeyMap & keymap, const Object & object)
//{
//	return EncodeUrlSegment(Detail::Key32ToString(keymap, object));
//}
//
//CString ArrayOfStringsToString(const KeyMap & keymap, const Object & object)
//{
//	CString output;
//
//	if (auto & values = Cast<Data::ArrayOfCStringProperty>(object)->value)
//	{
//		for (auto & i : values)
//		{
//			output.Append(EncodeUrlSegment(i));
//
//			output.Push(',');
//		}
//
//		output.Pop();
//	}
//
//	return output;
//}
//
//template <class TYPE> CString ArrayOfIntsToString(const KeyMap & keymap, const Object & object)
//{
//	CString output;
//
//	if (auto & values = Cast<ObjectOf<Array<TYPE>>>(object)->value)
//	{
//		for (auto & i : values)
//		{
//			output.Append(ToCString(i));
//
//			output.Push(',');
//		}
//
//		output.Pop();
//	}
//
//	return output;
//}

struct HttpQueryStringFormat : public StringFormat
{
	typedef Tuple <const TypeID&,Detail::ObjectToStringFn> TypeHandler;

	//static inline const TypeHandler kHandlers[] =
	//{
	//	{ REFLEX_TYPEID(CStringProperty), &CStringToString },
	//	{ REFLEX_TYPEID(WStringProperty), &WStringToString },
	//	{ REFLEX_TYPEID(Key32Property), &Key32ToStringUrlEncoded },

	//	{ REFLEX_TYPEID(Int32Property), &Detail::Int32ToString },
	//	{ REFLEX_TYPEID(Float32Property), &Detail::Float32ToString},
	//	{ REFLEX_TYPEID(BoolProperty), &Detail::BoolToString },

	//	{ REFLEX_TYPEID(ArrayOfCStringProperty), &ArrayOfStringsToString },
	//	
	//	{ REFLEX_TYPEID(ArrayOfUInt32Property), &ArrayOfIntsToString<UInt32> },
	//	{ REFLEX_TYPEID(ArrayOfUInt64Property), &ArrayOfIntsToString<UInt64> },
	//	{ REFLEX_TYPEID(ArrayOfInt32Property), &ArrayOfIntsToString<Int32> },
	//	{ REFLEX_TYPEID(ArrayOfInt64Property), &ArrayOfIntsToString<Int64> },
	//};

	//void OnReset(PropertySet & node) const override
	//{
	//	for (auto & i : kHandlers)
	//	{
	//		node.UnsetAll(i.a);
	//	}
	//}

	//bool SupportsType(TypeID type_id) const override
	//{
	//	return SearchValue<KeyCompare>(ToView(kHandlers), type_id);
	//}

	static bool PackNode(const KeyMap & keymap, const PropertySet & node, Archive & out)
	{
		bool set = false;

		for (auto & i : node.Iterate<CStringProperty>())
		{
			//auto type_id = i.key.type_id;

			auto & object = *i.value;

			//if (auto phandler = SearchValue<KeyCompare>(ToView(kHandlers), type_id))
			{
				out.Append(Data::Pack(EncodeUrlSegment(GetKey(keymap, i.key.id))));

				out.Push(UInt8('='));

				//CString buffer = phandler->b(keymap, object);

				//if (phandler->c) buffer = EncodeUrlSegment(buffer);

				out.Append(Data::Pack(object.value));

				out.Push(UInt8('&'));

				set = true;
			}
			//else if (type_id == REFLEX_TYPEID(PropertySet))
			//{
			//	set = Or(set, PackNode(keymap, Cast<PropertySet>(object), out));
			//}
			//else if (type_id == REFLEX_TYPEID(PropertySetArray))
			//{
			//	for (auto & i : Cast<PropertySetArray>(object)->value)
			//	{
			//		set = Or(set, PackNode(keymap, i, out));
			//	}
			//}
		}

		return set;
	}

	bool OnEncode(Archive & out, const PropertySet & root, UInt32 options) const override
	{
		auto keymap = GetKeyMap(root);

		if (PackNode(keymap, root, out))
		{ 
			out.Pop();
		}

		return true;
	}

	bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const override
	{
		auto keymap = AcquireKeyMap(out);

		auto parts = Split(Data::Unpack<CString::View>(in), '&');

		//bool typed = True(options & kHttpQueryOptionTyped);

		for (auto & i : parts)
		{
			if (auto idx = Search(i, '='))
			{
				auto [key_view,value] = Splice(i, idx.value);

				auto key_decoded = RegisterKey(keymap, DecodeUrlSegment(Trim(key_view)));

				auto value_decoded = DecodeUrlSegment(Trim(Nudge(value)));

				//if (typed && value_decoded)
				//{
				//	auto view = ToView(value_decoded);

				//	switch (view.GetFirst())
				//	{
				//	case '"':
				//		view = Nudge(view, 1);
				//		if (view) view.size--;
				//		SetCString(out, key_decoded, view);
				//		break;

				//	case '-':
				//	case '0':
				//	case '1':
				//	case '2':
				//	case '3':
				//	case '4':
				//	case '5':
				//	case '6':
				//	case '7':
				//	case '8':
				//	case '9':
				//		break;

				//	case 't':
				//	case 'T':
				//		if (CaseInsensitive::eq(view, Reflex::Detail::kFalseTrue[true]))
				//		{
				//			SetBool(out, key_decoded, true);
				//		}
				//		else
				//		{
				//			SetKey32(out, key_decoded, view);
				//		}
				//		break;

				//	case 'f':
				//	case 'F':
				//		if (CaseInsensitive::eq(view, Reflex::Detail::kFalseTrue[false]))
				//		{
				//			SetBool(out, key_decoded, false);
				//			break;
				//		}
				//		//fallthru

				//	default:
				//		SetKey32(out, key_decoded, view);
				//		break;
				//	}
				//}
				//else
				{
					SetCString(out, key_decoded, value_decoded);
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}
};

constexpr CString::View kSchemeDelimiter = "://";

const HttpQueryStringFormat g_http_query_string_format;

REFLEX_END_INTERNAL

Reflex::CString Reflex::Data::MakeUrl(const Url & url)
{
	CString rtn;

	if (url.scheme)
	{
		rtn.Append(url.scheme);
		rtn.Append(kSchemeDelimiter);
	}

	if (url.user.a)
	{
		rtn.Append(url.user.a);

		if (url.user.b)
		{
			rtn.Push(':');
			rtn.Append(url.user.b);
		}

		rtn.Push('@');
	}

	rtn.Append(url.domain);

	if (url.port)
	{
		rtn.Push(':');
		rtn.Append(ToCString(UInt32(url.port)));
	}

	if (url.resource)
	{
		rtn.Push('/');
		rtn.Append(url.resource);
	}

	//if (url.query)
	//{
	//	rtn.Push('?');
	//	rtn.Append(url.query);
	//}

	if (url.fragment)
	{
		rtn.Push('#');
		rtn.Append(url.fragment);
	}

	return rtn;
}

Reflex::Data::Url Reflex::Data::SplitUrl(const CString::View & string)
{
	Url url;

	CString::View authority;

	auto remain = string;

	if (auto pos = Search(remain, kSchemeDelimiter))
	{
		auto spliced = Splice(remain, pos.value);

		remain = Nudge(spliced.b, kSchemeDelimiter.size);

		url.scheme = spliced.a;
	}

	if (auto pos = ReverseSearch(remain, '#'))
	{
		auto spliced = Splice(remain, pos.value);

		remain = spliced.a;

		url.fragment = Nudge(spliced.b, 1);
	}

	//if (auto pos = ReverseSearch(remain, '?'))
	//{
	//	auto spliced = Splice(remain, pos.value);
	//	
	//	remain = spliced.a;
	//	
	//	url.query = Nudge(spliced.b, 1);
	//}

	if (auto pos = Search(remain, '/'))
	{
		auto spliced = Splice(remain, pos.value);

		authority = spliced.a;

		url.resource = Nudge(spliced.b, 1);
	}
	else
	{
		authority = remain;
	}

	if (auto pos = Search(authority, '@'))
	{
		auto spliced = Splice(authority, pos.value);

		auto userpass = spliced.a;

		authority = Nudge(spliced.b, 1);

		if (auto pos = Search(userpass, ':'))
		{
			auto spliced = Splice(userpass, pos.value);

			url.user = { spliced.a, Nudge(spliced.b, 1) };
		}
		else
		{
			url.user.a = userpass;
		}
	}

	if (auto pos = Search(authority, ':'))
	{
		auto spliced = Splice(authority, pos.value);

		url.domain = spliced.a;

		url.port = UInt16(ToUInt32(Nudge(spliced.b, 1))); // skip ':'
	}
	else
	{
		url.domain = authority;

		url.port = 0;
	}

	return url;
}

bool Reflex::Data::IsHttps(const CString::View & string)
{
	return True(Search(string, "https"));
}

Reflex::Pair <Reflex::CString::View> Reflex::Data::SplitUrlResource(const CString::View & url)
{
	if (auto pos = ReverseSearch(url, '?'))
	{
		auto spliced = Splice(url, pos.value);

		return { spliced.a, Nudge(spliced.b, 1) };
	}
	else
	{
		return { url, {} };
	}
}

Reflex::CString Reflex::Data::EncodeUrlSegment(const CString::View & view)
{
	CString rtn;

	rtn.Allocate(view.size);

	for (auto c : view)
	{
		switch (Detail::kChar2Type[Min<UInt8>(c, 127)])
		{
		case Detail::kCharTypeWord:		//includes '_'
		case Detail::kCharTypeNumber:
			rtn.Push(c);
			break;

		case Detail::kCharTypeSymbol:
			if (c == '-' || c == '~' || c == '.')
			{
				rtn.Push(c);

				continue;
			}
			//fallthru to hex...

		default:
		{
			auto ptr = Extend(rtn, 3).data;

			ptr[0] = '%';

			CString::Region output = { ptr + 1, 2 };

			Detail::BytesToHex(output, ToView(Reinterpret<UInt8>(c)));
		}
		break;
		}
	}

	return rtn;
}

Reflex::CString Reflex::Data::DecodeUrlSegment(const CString::View & view)
{
	if (Search(view, '%'))
	{
		Data::Archive rtn;

		rtn.Allocate(view.size * 2);

		for (UInt i = 0; i < view.size; ++i)
		{
			UInt8 c = view[i];

			if (c == '%')
			{
				if (i + 2 < view.size)  // Ensure there are two characters after '%'
				{
					const char hex[2] = { Lowercase(view[i + 1]), Lowercase(view[i + 2]) };

					Data::Archive::Region region = { &rtn.Push(1), 1 };

					Detail::HexToBytes(region, ToView(hex));

					i += 2;
				}
				else
				{
					break;
				}
			}
			else
			{
				rtn.Push(c);
			}
		}

		return Data::Unpack<CString>(rtn);
	}
	else
	{
		return view;
	}
}

const Reflex::ConstTRef <Reflex::Data::Format> Reflex::Data::kHttpQueryFormat = Reflex::Data::g_http_query_string_format;
