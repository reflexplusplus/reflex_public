#include "layerproperties.h"
#include "../../../behaviours/texteditimpl.h"




//
//todo
//
//support TextEditState -> cursor and selection_start, selection_range
//Text becomes abstract clas with Create methods
//Create(const WString & value, bool ellipsis = false)
//CreatePath(const WString & value)
//CreateWrapped(const WString & value)
//CreateMultiLine(



//
//layer

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

enum TextOverflow : UInt8
{
	kTextOverflowNone,

	kTextOverflowEllipsis,
	kTextOverflowEllipsisLeft,
	kTextOverflowPath,
	kTextOverflowShorten,
	kTextOverflowWrap,

	kNumTextOverflow
};

enum TextTransform : UInt8
{
	kTextTransformNone,

	kTextTransformLowercase,
	kTextTransformUppercase,
	kTextTransformMask,
	kTextTransformInitial,

	kNumTextTransform
};

GLX_DECLARE_ENUM(TextOverflow, "", "ellipsis", "ellipsis_left", "path", "shorten", "wrap");
GLX_DECLARE_ENUM(TextTransform, "", "lowercase", "uppercase", "mask", "initial");

//constexpr auto kPropertyGroupHasTransform = kPropertyGroupCustom0;
constexpr auto kPropertyGroupHasOverflow = kPropertyGroupCustom1;

Size CalculateTextSize(const Font & font, Float lineh, Float texth, const WString::View & text, bool multiline)
{
	Size size;

	if (multiline)
	{
		WString::View itr = text;

		while (itr)
		{
			auto line = Data::Detail::ReadLine(itr);

			size.w = Max(size.w, font.GetTextWidth(line));

			size.h += lineh;
		}

		size.h -= (lineh - texth);
	}
	else
	{
		size = { font.GetTextWidth(text), texth };
	}

	size.w = QuantiseUp(size.w, kPixelSize);

	return size;
}

void RemoveVowelsAndWhitespace(WString & s)
{
	auto Keep = [](wchar_t c) -> bool
	{
		switch (Lowercase(c))
		{
		case 'b':
		case 'c':
		case 'd':
		case 'f':
		case 'g':
		case 'h':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'v':
		case 'w':
		case 'x':
		//case 'y':
		case 'z':
			return true;

		default:
			return false;
		}
	};

	UInt len = s.GetSize();

	if (len <= 2) return;

	auto & workspace = g_library->cache.tempstring;

	workspace.Clear();

	workspace.SetSize(len);

	workspace[0] = s.GetFirst();

	UInt idx = 1;

	for (UInt i = 1; i < len; ++i)
	{
		wchar_t c = s[i];

		if (c == ' ')
		{
			i++;

			if (i < len)
			{
				workspace[idx++] = s[i];
			}
		}
		else if (Keep(c))
		{
			workspace[idx++] = c;
		}
	}

	workspace.SetSize(idx);

	s.Swap(workspace);
}

void WrapText(const Font & font, Float & maxwidth, const WString::View & string, Array <WString> & output)
{
	struct WordSplitter
	{
		WordSplitter(const WString::View & string)
			: string(string)
			, m_space(L' ')
			, m_length(string.size)
			, m_position(0)
		{
		}

		bool GetNext(WString & word, bool & last)
		{
			word.Clear();

			if (auto pos = Search(Mid(string, m_position), m_space))
			{
				WString::View ref(string.data + m_position, pos.value);

				word.Append(ref);

				m_position += pos.value + 1;

				last = false;

				return true;
			}
			else
			{
				UInt wordlen = m_length - m_position;

				WString::View ref(string.data + m_position, wordlen);

				word.Append(ref);

				m_position = m_length;

				last = true;

				return wordlen != 0;
			}
		}

		const WString::View string;

		WChar m_space;

		UInt m_length;

		UInt m_position;
	};

	output.Clear();

	if (maxwidth > 0.0f)
	{
		WordSplitter splitter(string);

		WString::View space = L" ";

		Float spacew = font.GetTextWidth(space);

		Float linew = 0.0f;

		WString * line = &output.Push();

		WString word;

		Float w = 0.0f;

		bool last = false;

		while (splitter.GetNext(word, last))
		{
			Float wordw = font.GetTextWidth(word);

			if (maxwidth < (linew + wordw))
			{
				if (*line) line->Pop();

				line = &output.Push();

				w = Max(w, linew);

				linew = 0.0f;
			}

			if (last)
			{
				linew += wordw;

				w = Max(w, linew);

				line->Append(word);
			}
			else
			{
				linew += wordw + spacew;

				line->Append(Join(word, space));
			}
		}

		maxwidth = Max(maxwidth, w);

		if (!output.GetFirst()) output.Remove(0);
	}
}

Float CalculateLineHeight(Pair <Float> metrics, Float line_height, Float line_space)
{
	return SnapToPixels(((metrics.a + metrics.b) * line_height) + line_space);
}

struct StandardTextProperties : public StandardPropertiesWithColour
{
	REFLEX_NOINLINE static void Register(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterReferenceProperty<Font>(schema, REFLEX_OFFSETOF(StandardTextProperties, font), "font");

		schema.RegisterProperty({ UInt16(REFLEX_OFFSETOF(StandardTextProperties, value)), kPropertyGroupNone, kBindStageAccommodate, "value", MakeAddress<Text>("value"), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			*Cast<ConstReference<Text>>(adr) = Cast<Text>(object);
		} });

		schema.RegisterProperty({ UInt16(REFLEX_OFFSETOF(StandardTextProperties, value)), kPropertyGroupNone, kBindStageNone, "value", MakeAddress<Data::Key32Property>("value"), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto id = Cast<Data::Key32Property>(object)->value;

			auto p = FindResource<Data::CStringProperty>(st_current_style, id);

			auto text = REFLEX_CREATE(Text, ToWString(p->value));

			*Cast<ConstReference<Text>>(adr) = text;
		} });

		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(StandardTextProperties, justify), "justify", kPropertyGroupNone, kBindStageNone);
		RegisterAxisProperty(schema, REFLEX_OFFSETOF(StandardTextProperties, autofit), kAutoFit);
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(StandardTextProperties, line_space), "line_space");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(StandardTextProperties, line_height), "line_height");
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(StandardTextProperties, transform), "transform", &kTextTransform, kPropertyGroupNone, kBindStageBuildLayout);
	}

	ConstReference <Font> font;
	ConstReference <Text> value;

	Float32 line_space = 0.0f;
	Float32 line_height = 1.0f;
	Pair <Orientation> justify = { kOrientationNear, kOrientationCenter };
	AxisProperty autofit = kAxisPropertyXY;
	TextTransform transform = kTextTransformNone;
	TextOverflow overflow = kTextOverflowNone;
};

struct StandardTextScratch : public StandardScratch
{
	static void Init(GenericLayer & self, const StandardTextProperties & properties, StandardTextScratch & scratch, GenericLayer::VTable & vtable)
	{
		auto font_size = properties.font->GetHeightAndTail();

		scratch.metrics = font_size;

		auto line_space = properties.line_space;

		scratch.lineh = CalculateLineHeight(font_size, properties.line_height, line_space);

		scratch.texth = scratch.lineh - (font_size.b + line_space);

		scratch.justify = OrientationToAlignment(properties.justify);
	}

	StandardTextScratch()
		: transform_buffer(REFLEX_CREATE(ObjectOf<WString>))
	{
	}

	StandardTextScratch(const StandardTextScratch & scratch)
		: StandardScratch(scratch),
		metrics(scratch.metrics),
		lineh(scratch.lineh),
		texth(scratch.texth),
		justify(scratch.justify),
		multiline(scratch.multiline),
		textsize(scratch.textsize),
		transform_buffer(REFLEX_CREATE(ObjectOf<WString>))
	{
		REFLEX_ASSERT(scratch.transform_buffer->value.Empty());
	}

	StandardTextScratch(StandardTextScratch&&c) = delete;

	StandardTextScratch & operator=(const StandardTextScratch&c) = delete;

	Pair <Float32> metrics;

	Float32 lineh, texth;

	Alignment justify = kAlignmentLeft;

	bool multiline = false;

	Size textsize;

	Reference < ObjectOf <WString> > transform_buffer;	//newed to shared with TextEditBehaviour, TODO optimise so only for TextEdit
};

struct TextImpl
{
	using Properties = StandardTextProperties;

	struct Scratch;

	using OverflowFn = FunctionPointer <Float(const Properties&, Scratch&, Float)>;
	using TransformFn = FunctionPointer <void(WString & buffer, const WString::View &)>;

	struct Scratch : public StandardTextScratch
	{
		Array < ConstReference <Graphic> > lines;
	};

	
	static Float OverflowNone(const Properties & properties, Scratch & scratch, Float w);
	template <TextOverflow MODE> static Float OverflowTruncate(const Properties & properties, Scratch & scratch, Float w);
	static Float OverflowWrap(const Properties & properties, Scratch & scratch, Float w);

	static constexpr OverflowFn kOverflowFns[kNumTextOverflow] = { &OverflowNone, &OverflowTruncate<kTextOverflowEllipsis>, &OverflowTruncate<kTextOverflowEllipsisLeft>, &OverflowTruncate<kTextOverflowPath>, &OverflowTruncate<kTextOverflowShorten>, &OverflowWrap };


	static void TransformNone(WString & buffer, const WString::View & w) { buffer = w; }
	static void Lowercase(WString & buffer, const WString::View & view) { buffer = Reflex::Lowercase(view); }
	static void Uppercase(WString & buffer, const WString::View & view) { buffer = Reflex::Uppercase(view); }
	static void Mask(WString & buffer, const WString::View & view) { buffer.SetSize(view.size); buffer.Fill(WChar(9679)); }
	static void Initial(WString & buffer, const WString::View & text) { buffer = Left<true>(text, 1); }

	static constexpr TransformFn kTransformFns[kNumTextTransform] = { &TransformNone, &Lowercase, &Uppercase, &Mask, &Initial };


	//template <class SCRATCH> static void OnBindImpl(const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
	//{
	//	auto properties = data.GetProperties<StandardTextProperties>();

	//	if constexpr (IsType<SCRATCH, Scratch>::value)
	//	{
	//		auto scratch = data.GetScratch<Scratch>(layer);

	//		scratch->overflowfn = kOverflowFns[properties->overflow];

	//		//switch (properties->overflow.value)
	//		//{
	//		//case K32("ellipsis"):
	//		//	scratch->overflowfn = &Truncate<K32("ellipsis")>;
	//		//	break;

	//		//case K32("ellipsis_left"):
	//		//	scratch->overflowfn = &Truncate<K32("ellipsis_left")>;
	//		//	break;

	//		//case K32("path"):
	//		//	scratch->overflowfn = &Truncate<K32("path")>;
	//		//	break;

	//		//case K32("wrap"):
	//		//	scratch->overflowfn = [](const Properties & properties, Scratch & scratch, Float w)
	//		//	{
	//		//		auto & font = *properties.font;

	//		//		auto & wrapped = g_library->cache.lines;

	//		//		WrapText(font, w, scratch.transform_buffer->value, wrapped);

	//		//		return LayoutTextBlock(properties, scratch, wrapped);
	//		//	};
	//		//	break;

	//		//case K32("shorten"):
	//		//	scratch->overflowfn = &Truncate<K32("shorten")>;
	//		//	break;

	//		//default:
	//		//	scratch->overflowfn = [](const Properties & properties, Scratch & scratch, Float w)
	//		//	{
	//		//		return LayoutTextBlock(properties, scratch, { scratch.transform_buffer->value });
	//		//	};
	//		//};
	//	}

	//	//if (!(layer.propertyflags & kPropertyGroupHasInlineValue))
	//	//{
	//	//	RemoveConst(properties->value) = GetProperty<Text>(object, kvalue);
	//	//}
	//}

	REFLEX_INLINE static void OnAccommodateImpl(const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
	{
		auto properties = data.GetProperties<StandardTextProperties>();

		auto scratch = data.GetScratch<StandardTextScratch>(layer);

		auto & text = *properties->value;

		auto view = text.GetView();

		
		scratch->multiline = text.IsMultiLine();

		auto & textsize = scratch->textsize;

		auto & buffer = scratch->transform_buffer->value;

		kTransformFns[properties->transform](buffer, view);

		textsize = CalculateTextSize(properties->font, scratch->lineh, scratch->texth, buffer, scratch->multiline);

		//if constexpr (HAS_TRANSFORM)
		//{
		//	switch (properties->transform.value)
		//	{
		//	case K32("uppercase"):
		//		buffer = Uppercase(view);
		//		break;

		//	case K32("lowercase"):
		//		buffer = Lowercase(view);
		//		break;

		//	case K32("mask"):
		//		buffer.SetSize(view.size);
		//		buffer.Fill(WChar(9679));
		//		break;

		//	case K32("initial"):
		//		buffer = Initial(view);
		//		break;

		//	default:
		//		buffer = view;
		//		break;
		//	}
		//}
		//else
		//{
		//	buffer = view;
		//}

		if (auto autofit = properties->autofit)
		{
			contentsize = textsize * kAxisToSize[autofit];

			contentsize += Sum(properties->indent);
		}
	}

	template <bool HAS_OVERFLOW> static void OnAlignImpl(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
	{
		auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

		Float32 unused;

		StandardIndent(layer, data, size, unused);

		const auto & text = scratch->transform_buffer->value;

		if (scratch->multiline)
		{
			FunctionPointer <void(const Font &, Float, Array <WString> &, const WString::View &)> addline = [](const Font & font, Float w, Array <WString> & lines, const WString::View & line)
			{
				lines.Push(line);
			};

			auto font = properties->font;

			Float w;

			if constexpr (HAS_OVERFLOW)
			{
				w = scratch->inner.size.w;

				if (properties->overflow == kTextOverflowWrap && (scratch->textsize.w > w) && (w > 1.0f))
				{
					addline = [](const Font & font, Float w, Array <WString> & lines, const WString::View & line)
					{
						auto & wrapped = g_library->cache.lines;

						WrapText(font, w, line, wrapped);

						lines.Append(wrapped);
					};
				}
			}
			else
			{
				w = {};
			}

			Array <WString> lines;

			auto view = ToView(text);

			while (view)
			{
				auto line = Data::Detail::ReadLine(view);

				addline(properties->font, w, lines, line);
			}

			contenth = LayoutTextBlock(properties, scratch, lines);
		}
		else
		{
			if constexpr (HAS_OVERFLOW)
			{
				auto & inner = scratch->inner;

				auto w = inner.size.w;

				if (scratch->textsize.w > w && (w > 1.0f))
				{
					contenth = kOverflowFns[properties->overflow](properties, scratch, w);
				}
				else
				{
					contenth = LayoutTextBlock(properties, scratch, { text });
				}
			}
			else
			{
				contenth = LayoutTextBlock(properties, scratch, { text });
			}
		}

		constexpr Float32 kMult[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

		contenth += AxisSum<YAxis>(properties->indent);

		contenth *= kMult[properties->autofit];
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		StandardTextProperties::Register(schema);

		schema.RegisterProperty({ UInt16(REFLEX_OFFSETOF(Properties, value)), kPropertyGroupNone, kBindStageNone, "value", MakeAddress<Data::CStringProperty>("value"), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			*Cast<ConstReference<Text>>(adr) = REFLEX_CREATE(Text, ToWString(Cast<Data::CStringProperty>(object)->value), false);
		} });

		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, overflow), "overflow", &kTextOverflow, kPropertyGroupHasOverflow, kBindStageBuildLayout);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			auto scratch = Cast<Scratch>(pscratch);

			StandardTextScratch::Init(self, *properties, *scratch, vtable);

			if ((properties->autofit | kAxisPropertyY) && (properties->overflow == kTextOverflowWrap))
			{
				RemoveConst(self.flags) &= ~Layer::kOptimisationFlagNotResponsive;	//CLEAN UP as param?
			}

			//vtable.OnBind = &TextImpl::OnBindImpl<Scratch>;

			vtable.OnAccommodate = &TextImpl::OnAccommodateImpl;

			vtable.OnAlign = (self.propertyflags & kPropertyGroupHasOverflow) ? &TextImpl::OnAlignImpl<true> : &TextImpl::OnAlignImpl<false>;

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				struct GraphicImpl : public Graphic
				{
					GraphicImpl(const Array < ConstReference <Graphic> > & lines, const Colour & colour) : lines(lines), colour(colour) {}

					void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
					{
						auto recolour = GraphicImpl::colour * colour;

						for (auto & i : lines) i->Render(transform, recolour);
					}

					const Array < ConstReference <Graphic> > & lines;

					Colour colour;
				};

				return REFLEX_CREATE(GraphicImpl, scratch->lines, properties->colour);
			};
		});
	}

	REFLEX_NOINLINE static Float LayoutTextBlock(const Properties & properties, Scratch & scratch, const ArrayView <WString> & lines)
	{
		auto font = properties.font;

		const auto & inner = scratch.inner;

		auto lineh = scratch.lineh;

		Float total_h = lineh * lines.size;

		auto [x_orientation, y_orientation] = properties.justify;

		auto [ascent, tail] = properties.font->GetHeightAndTail();

		Float y = inner.origin.y + SnapToPixels(Align1D(inner.size.h, total_h, y_orientation) + Align1D(lineh, ascent, y_orientation)) + ascent;

		Float inner_x = inner.origin.x;

		Float inner_w = inner.size.w;

		scratch.lines.Clear();

		for (auto & i : lines)
		{
			Float x = inner_x + SnapToPixels(Align1D(inner_w, font->GetTextWidth(i), x_orientation));

			Point position = { x, y };

			scratch.lines.Push(font->CreateText(i, position).a);

			y += lineh;
		}

		return total_h;
	}
};

Float TextImpl::OverflowNone(const Properties & properties, Scratch & scratch, Float w)
{
	return LayoutTextBlock(properties, scratch, { scratch.transform_buffer->value });
}

template <TextOverflow ID> Float TextImpl::OverflowTruncate(const Properties & properties, Scratch & scratch, Float w)
{
	auto & font = *properties.font;

	WString copy = scratch.transform_buffer->value;

	if constexpr (ID == kTextOverflowEllipsis)
	{
		TruncateRight(font, w, copy);
	}
	else if constexpr (ID == kTextOverflowEllipsisLeft)
	{
		TruncateLeft(font, w, copy);
	}
	else if constexpr (ID == kTextOverflowPath)
	{
		TruncatePath(font, w, copy);
	}
	else if constexpr (ID == kTextOverflowShorten)
	{
		if (font.GetTextWidth(copy) > w) RemoveVowelsAndWhitespace(copy);

		TruncateRight(font, w, copy);
	}

	return LayoutTextBlock(properties, scratch, { copy });
}

Float TextImpl::OverflowWrap(const Properties & properties, Scratch & scratch, Float w)
{
	auto & font = *properties.font;

	auto & wrapped = g_library->cache.lines;

	WrapText(font, w, scratch.transform_buffer->value, wrapped);

	return LayoutTextBlock(properties, scratch, wrapped);
}

struct TextEditImpl
{
	struct Properties : public StandardTextProperties
	{
		Colour selection_colour = kWhite;

		Colour selected_text_colour = kBlack;
	};

	struct Scratch : public StandardTextScratch
	{
		Reference <TextEditBehaviour> state;

		TRef <AbstractViewPort> viewport;

		Size text_size;

		Float caret_height, caret_offset;

		Array < Tuple < ConstReference <Graphic>, Colour > > segments;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		StandardTextProperties::Register(schema);

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, selection_colour), "selection_color");

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, selection_colour), "selection_colour");

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, selected_text_colour), "selected_text_color");

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, selected_text_colour), "selected_text_colour");

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			auto scratch = Cast<Scratch>(pscratch);

			StandardTextScratch::Init(self, *properties, *scratch, vtable);

			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				auto dlg = data.owner->QueryDelegate(TextEditBehaviour::kDynamicTypeInfo);

				auto state = Cast<TextEditBehaviourWithState>(dlg ? dlg : &TextEditBehaviour::null);

				scratch->state = state;

				state->m_font = properties->font;

				state->m_transformed = scratch->transform_buffer;

				//TextImpl::OnBindImpl<Scratch>(layer, data, object);
			};

			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				//auto accommodatefn = &TextImpl::OnAccommodateImpl;

				TextImpl::OnAccommodateImpl(layer, data, contentsize);

				auto & font = *properties->font;

				scratch->viewport = GetContainingViewPort(data.owner);

				auto metrics = font.GetHeightAndTail();

				Float extend = SnapToPixels(properties->line_space * 0.5f);

				scratch->caret_height = metrics.a + metrics.b + (extend * 2.0f);

				scratch->caret_offset = -(metrics.a + extend);
			};

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				Float32 unused;

				StandardIndent(layer, data, size, unused);


				auto state = Cast<TextEditBehaviourWithState>(scratch->state);

				auto caretpos = state->m_caret;


				auto & font = *properties->font;

				auto & colour = properties->colour;

				auto & segments = scratch->segments;

				const auto lineh = scratch->lineh;

				auto textview = ToView(scratch->transform_buffer->value);

				auto & view = *scratch->viewport;


				const auto & data_inner = scratch->inner;

				auto textsize = CalculateTextSize(font, lineh, scratch->texth, textview, scratch->multiline);

				auto data_position = Align(data_inner.size, textsize, scratch->justify) + data_inner.origin;

				state->m_lineh = lineh;

				state->m_text_rect = { data_position, textsize };

				data_position = SnapToPixels(data_position);

				//data_position.y = Reflex::RoundNearest(data_position.y);


				auto metrics = scratch->metrics;

				data_position.y += metrics.a;


				auto [vo, vr] = view.GetView();

				vo.y -= CalculateAbs(view.GetContent(), data.owner).a.y;

				auto vh = Max(vr.h, view.GetBody()->GetRect().size.h);

				Float32 clip_top = vo.y - (metrics.a + 1.0f);

				Float32 clip_bottom = vo.y + vh + (metrics.a + metrics.b + 1.0f);

				Point position = data_position;

				Rect caret = { { position.x, 0.0f }, { 1.0f, scratch->caret_height } };

				Float32 caretline = 0;

				Float32 nline = 0;

				const auto start = textview.data;

				auto textitr = textview;

				segments.Clear();

				while (textitr)
				{
					auto line = Data::Detail::ReadLine(textitr);

					Int line_start = Int(line.data - start);

					if (position.y > clip_top && position.y < clip_bottom)
					{
						auto selection = state->m_selection;

						Int select_start = Reflex::Max<Int>(selection.a, line_start);

						Int select_length = Reflex::Min<Int>(selection.a + selection.b, line_start + line.size) - select_start;

						if (select_length > 0)
						{
							segments.Allocate(segments.GetSize() + 4);


							select_start -= line_start;


							Point t = position;

							auto left = font.CreateText(Left(line, select_start), t);

							t.x += left.b;

							segments.Push<kAllocateNone>({ left.a, colour });


							auto mid = font.CreateText(Mid<false>(line, select_start, select_length), t);

							Float w = mid.b;

							GLX_GET_POINT_WORKSPACE(points);

							AddRectFill(points, { { t.x, t.y + scratch->caret_offset }, { w, scratch->caret_height } });

							auto fill = CreateGraphic(points);

							segments.Push<kAllocateNone>({ fill, properties->selection_colour });

							segments.Push<kAllocateNone>({ mid.a, properties->selected_text_colour });

							t.x += w;


							auto right = font.CreateText(Right<false>(line, line.size - (select_start + select_length)), t);

							segments.Push<kAllocateNone>({ right.a, colour });
						}
						else
						{
							auto text = font.CreateText(line, position);

							segments.Push({ text.a, colour });
						}
					}

					position.y += lineh;

					if (Reflex::Inside<UInt>(caretpos, line_start, line.size + 1))
					{
						caret.origin.x += font.GetTextWidth(WString::View(line.data, caretpos - line_start))/* + 0.5f*/;

						caretline = nline;
					}

					nline += 1.0f;
				}

				if (textview && caretpos == textview.size)
				{
					if (textview[textview.size - 1] == char(10))
					{
						caretline = nline;
					}
				}

				if (state->m_show_caret)
				{
					caret.origin.y = data_position.y + scratch->caret_offset + (caretline * lineh);

					GLX_GET_POINT_WORKSPACE(points);

					AddRectFill(points, caret);

					segments.Push({ CreateGraphic(points), colour });
				}
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				struct GraphicImpl : public Graphic
				{
					GraphicImpl(const Scratch & scratch) : scratch(scratch) {}

					void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
					{
						for (auto & i : scratch.segments) i.a->Render(transform, colour * i.b);
					}

					const Scratch & scratch;
				};

				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				return REFLEX_CREATE(GraphicImpl, scratch);
			};
		});
	};
};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_text_layers[]
{
	{ "Text", &GenericLayer::CreateSchema<TextImpl>, &GenericLayer::Create<TextImpl> },
	{ "TextEdit", &GenericLayer::CreateSchema<TextEditImpl>, &GenericLayer::Create<TextEditImpl> }
};
