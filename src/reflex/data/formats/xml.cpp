#include "xml.h"
#include "../tokeniser.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct XmlParser
{
	static constexpr UInt8 kEOL = 10;

	typedef Array < Pair <CString::View> > Attributes;

	typedef Function <TRef<Object>(Object &, const CString::View &, const Attributes::View &)> Callback;


	~XmlParser();

	bool Parse(const Archive::View & data, Object & root, bool xml_mode, const Callback & callback);


	template <bool NESTED> void CallClient(const CString::View & tag, const Attributes::View & attributes);

	template <bool XML> static void SplitProperties(XmlParser & self, const CString::View & text);

	bool GetNext(decltype(&XmlParser::SplitProperties<true>) & format, CString::View & itr, CString::View & tag);

	static CString::View ParseQuote(CString::View & segment);


	Callback m_callback;

	Array <TRef<Object>> m_stack;


	static constexpr CString::View kvalue = "value";
};

XmlParser::~XmlParser()
{
	REFLEX_RFOREACH(i, m_stack)
	{
		Reflex::Detail::SilentRelease(i);
	}
}

bool XmlParser::Parse(const Archive::View & data, Object & root, bool xml_mode, const Callback & callback)
{
	m_callback = callback;

	m_stack.Allocate(4);

	Retain(root);

	m_stack.Push<kAllocateNone>(root);

	try
	{
		auto split_fn = xml_mode ? &XmlParser::SplitProperties<true> : &XmlParser::SplitProperties<false>;

		CString::View itr(Reinterpret<char>(data.data), data.size);

		CString::View tag;

		while (GetNext(split_fn, itr, tag))
		{
			auto ptr = tag.data;

			if (ptr[0] == '?' || ptr[0] == '!')
			{
				continue;
			}
			else if (ptr[0] == '/')
			{
				if (m_stack.Empty()) throw ToView("XML mismatched />");

				auto top = m_stack.GetLast();
				
				Reflex::Detail::SilentRelease(top);

				m_stack.Pop();
			}
			else if (m_stack && tag)
			{
				split_fn(*this, tag);
			}
		}

		return true;
	}
	catch (CString::View &)
	{
		return true;
	}
}

template <bool NESTED> void XmlParser::CallClient(const CString::View & tag, const Attributes::View & attributes)
{
	auto & current = *m_stack.GetLast();

	if constexpr (NESTED)
	{
		auto top = m_callback(current, tag, attributes);

		Retain(top);

		m_stack.Push(top);
	}
	else
	{
		AutoRelease(m_callback(current, tag, attributes));
	}
}

bool XmlParser::GetNext(decltype(&XmlParser::SplitProperties<true>) & format, CString::View & itr, CString::View & tag)
{
	auto segment = itr;

	REFLEX_LOOP_PTR(segment.data, ptr, segment.size)
	{
		if (*ptr == '<')
		{
			auto start = UInt(ptr - segment.data) + 1;

			if (auto idx = Detail::SearchCharacter(Mid(segment, start), '>'))
			{
				UInt32 end = start + idx.value;

				format = &XmlParser::SplitProperties<true>;

				itr = CString::View(segment.data + end + 1, segment.size - (end + 1));

				tag = CString::View(segment.data + start, end - start);

				tag = Trim(tag);

				return true;
			}
		}
		else if (*ptr == '#')
		{
			auto start = UInt(ptr - segment.data) + 1;

			format = &XmlParser::SplitProperties<false>;

			tag.data = segment.data + start;

			if (auto idx = Detail::SearchCharacter(Mid(segment, start), kEOL))
			{
				UInt32 end = start + idx.value;
								
				itr = CString::View(segment.data + end + 1, segment.size - (end + 1));
				
				tag.size = end - start;
			}
			else
			{
				itr = {};
				
				tag.size = segment.size - start;
			}
			
			tag = Trim(tag);

			return true;
		}
		else if (IsWhiteSpace(*ptr))
		{
			continue;
		}
	}

	format = &XmlParser::SplitProperties<true>;

	return false;
}

template <bool XML> void XmlParser::SplitProperties(XmlParser & self, const CString::View & segment)
{
	CString::View ref = segment;

	bool nested_xml = false;

	if constexpr (XML)
	{
		if (ref.data[ref.size - 1] == '/')
		{
			nested_xml = false;

			ref.size--;
		}
		else
		{
			nested_xml = true;
		}
	}

	CString::View tag(ref.data, ref.size);

	struct SpaceOrLineBreak
	{
		static bool eq(char a, char b) { return IsWhiteSpace(a); }
	};

	if (auto idx = Search<SpaceOrLineBreak>(ref, char(0)))
	{
		if (idx.value == 0) throw ToView("error");


		//name

		tag = { ref.data, idx.value };

		ref.data += idx.value;

		ref.size -= idx.value;


		//properties

		if constexpr (XML)
		{
			Attributes attributes;

			attributes.Allocate(4);

			while (ref)
			{
				ref = TrimLeft(ref);

				if (auto equal = Detail::SearchCharacter(ref, '='))
				{
					UInt pos = equal.value;

					CString::View name(ref.data, pos);

					name = TrimRight(name);

					pos++;

					ref.data += pos;

					ref.size -= pos;

					ref = TrimLeft(ref);

					if (ref) attributes.Push({name, ParseQuote(ref)});
				}
				else
				{
					break;
				}
			}

			if (nested_xml)
			{
				self.CallClient<true>(tag, attributes);
			}
			else
			{
				self.CallClient<false>(tag, attributes);
			}
		}
		else
		{
			self.CallClient<false>(tag, { { kvalue, Trim(ref, &IsCharacter<' '>) } });

			return;
		}
	}
	else if constexpr (XML)
	{
		if (nested_xml)
		{
			self.CallClient<true>(tag, {});
		}
		else
		{
			self.CallClient<false>(tag, {});
		}
	}
	else
	{
		self.CallClient<false>(tag, {});
	}
}

CString::View XmlParser::ParseQuote(CString::View & text)
{
	auto segment = text;

	char quotes[] = { '"', '\'' };

	REFLEX_LOOP_PTR(quotes + 0, ptr, 2)
	{
		auto quote = *ptr;

		if (text[0] == quote)
		{
			constexpr UInt32 start = 1;

			if (auto idx = Detail::SearchCharacter(Mid(segment, start), quote))
			{
				UInt32 end = start + idx.value;

				text = CString::View(segment.data + end + 1, segment.size - (end + 1));

				return CString::View(segment.data + start, end - start);	//TODO TRIM WHITE SPACE
			}
		}
	}

	if (auto idx = Detail::SearchCharacter(segment, ' '))
	{
		segment.size = idx.value;

		text.data += segment.size;

		text.size -= segment.size;
	}
	else
	{
		segment = TrimRight(segment);

		text.size = 0;
	}

	return segment;// CString::View();
}

TypeID const * const AbstractMarkupFormat::kSupportedTypes[] =
{
	&REFLEX_TYPEID(KeyMap),
	&REFLEX_TYPEID(PropertySetArray),
	&REFLEX_TYPEID(CStringProperty),
};

AbstractMarkupFormat::AbstractMarkupFormat(const Detail::StandardPropertySheetInterface & interface)
{
	m_types[0] = { REFLEX_TYPEID(CStringProperty), [](const KeyMap & keymap, const Object & cstring_object)
	{
		return Cast<CStringProperty>(cstring_object)->value;
	} };

	m_types[1] = { REFLEX_TYPEID(BoolProperty), &Detail::BoolToString };

	m_types[2] = { REFLEX_TYPEID(Int32Property), &Detail::Int32ToString };

	m_types[3] = { REFLEX_TYPEID(Float32Property), &Detail::Float32ToString };
}

void AbstractMarkupFormat::OnReset(PropertySet & node) const
{
	UnsetAll<PropertySetArray>(node);
	UnsetAll<CStringProperty>(node);
}

bool AbstractMarkupFormat::SupportsType(TypeID type_id) const
{
	for (auto & i : kSupportedTypes)
	{
		if (*i == type_id) return true;
	}

	return false;
}

void AbstractMarkupFormat::Validate(const PropertySet & root)
{
#if	(REFLEX_DEBUG)
	for (auto & i : root.Iterate()) REFLEX_ASSERT(SupportsType(i.key.type_id));
#endif
	
	for (auto & i : GetXmlNodes(root))
	{
		Validate(i);
	}
}

bool AbstractMarkupFormat::OnEncode(Archive & out, const PropertySet & root, UInt options) const
{
	REFLEX_LOCAL(void, Flush)(Archive & stream, Archive & buffer)
	{
		stream.Append(buffer);

		buffer.Clear();
	}
	REFLEX_END

	typedef Sequence < bool, Pair<CString::View,CString> > Sorted;

	REFLEX_LOCAL(Sorted, GetAttributes)(const ArrayView <TypeHandler> & types, const KeyMap & keymap, const PropertySet & node)
	{
		Sorted sorted;

		for (auto & type : types)
		{
			for (auto & i : node.Iterate(type.a))
			{
				auto key = i.key.id;

				CString string = type.b(keymap, i.value);

				switch (key.value)
				{
				case kid.value:
					sorted.Insert(0, { "id", std::move(string) });
					break;

				case ktag.value:
					break;

				default:
					if (auto text = GetKey(keymap, key))
					{
						sorted.Insert(1, { text, std::move(string) });
					}
					else
					{
						File::output.Warn("File::iXML missign keymap", string);
					}
					break;
				}
			}
		}

		return sorted;
	}
	REFLEX_END

	REFLEX_LOCAL(void, Parse)(const ArrayView <TypeHandler> & types, const KeyMap & keymap, const PropertySet & node, Archive & indent, Archive & stream)
	{
		if (auto tag = GetXmlTag(node))
		{
			Archive line = indent;

			line.Push('<');

			Append(line, Data::Pack(tag));

			auto sorted = GetAttributes::Call(types, keymap, node);

			CString temp;

			for (auto & i : sorted)
			{
				auto & pair = i.value;

				//WORKAROUND
				//needed converting from propertysheet to xml

				if (Detail::SearchCharacter(pair.b, char(XmlParser::kEOL)))
				{
					temp.Clear();

					auto parts = Split(pair.b, char(XmlParser::kEOL));

					for (auto & ii : parts) temp = Join(temp, Trim(ii));

					pair.b = temp;
				}

				Append(line, Data::Pack(Join(kSpace, pair.a, kValueOpen, pair.b, '"')));
			}

			if (auto children = GetXmlNodes(node))
			{
				Append(line, Join(UInt8('>'), XmlParser::kEOL, XmlParser::kEOL));

				Flush::Call(stream, line);

				indent.Push(9);

				for (auto & i : children)
				{
					Parse::Call(types, keymap, i, indent, stream);
				}

				indent.Pop();

				Append(line, indent);

				Append(line, Data::Pack(ToView("</")));

				Append(line, Data::Pack(tag));

				Append(line, Join(UInt8('>'), XmlParser::kEOL, XmlParser::kEOL));

				Flush::Call(stream, line);
			}
			else
			{
				Append(line, Join(Data::Pack(ToView("/>")), XmlParser::kEOL, XmlParser::kEOL));

				Flush::Call(stream, line);
			}

			line.Push(XmlParser::kEOL);
		}
	}
	REFLEX_END

	auto keymap = GetKeyMap(root);

	Archive indent;

	Parse::Call(ToView(m_types), keymap, root, indent, out);

	return true;
}

bool XML::OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const
{
	auto keymap = AcquireKeyMap(out);

	XmlParser parser;

	bool ok = parser.Parse(in, out, true, [keymap](Object & object, const CString::View & tag, const XmlParser::Attributes::View & attributes) -> TRef <Object>
	{
		auto node = Cast<PropertySet>(object);

		auto child = AddXmlNode(AcquireXmlNodes(node), tag);

		for (auto & i : attributes)
		{
			SetCString(child, RegisterKey(keymap, i.a), i.b);
		}

		return child;
	});

	if (ok)
	{
		auto children = GetXmlNodes(out);

		if (children.size == 1)
		{
			auto keymap_retainer = AutoRelease(keymap);

			auto root = AutoRelease(children.GetFirst());

			out = *root;

			out.SetProperty(kkeymap, keymap);

			return true;
		}
	}

	UnsetCString(out, ktag);

	out.UnsetProperty<PropertySetArray>({});

	return false;
}

bool Markup::OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const
{
	XmlParser parser;

	auto keymap = AcquireKeyMap(out);

	return parser.Parse(in, out, false, [keymap](Object & object, const CString::View & tag, const XmlParser::Attributes::View & attributes) -> TRef <Object>
	{
		auto node = Cast<PropertySet>(object);

		auto children = AcquirePropertySetArray(node, {});

		auto item = AddPropertySet(children);

		SetCString(item, ktag, tag);
		SetKey32(item, K32("type"), RegisterKey(keymap, tag));	//LEGACY

		for (auto & i : attributes)
		{
			SetCString(item, i.a, i.b);
		}

		return item;
	});
}

bool Markup::OnEncode(Archive & out, const PropertySet & in, UInt32 options) const
{
	auto keymap = GetKeyMap(in);
	
	auto nodes = GetPropertySetArray(in, {});
	
	for (auto & i : nodes)
	{
		if (auto ptag = keymap->value.Search(GetKey32(i, K32("type"))))
		{
			if (auto nested = GetPropertySetArray(i, {}))
			{
				//todo
			}
			else
			{
				out.Push('#');
				
				out.Append(Data::Pack(*ptag));
				
				out.Push(' ');
				
				auto value = GetCString(i, K32("value"));
				
				out.Append(Data::Pack(value));
				
				WriteLine(out);
			}
		}
	}
	
	return true;
}

REFLEX_END_INTERNAL
