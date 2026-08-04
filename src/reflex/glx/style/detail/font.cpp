#include "reflex/glx/detail/font.h"
#include "../stylesheet.h"
#include "cstyle.h"
#include "../../freetype.h"

//character sets
//0 - 255: western/euro
//4352: korean
//12352: hiragana
//12448: katakana
//19968-40959: kanji

//http://nihongo.monash.edu/jouyoukanji.html




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct NoStringRenderer : public System::Renderer::Graphic
{
	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override {}
} 

gEmptyStringRenderer;

struct Freetype : public Reflex::Object
{
	typedef Tuple <WChar, WChar, WChar, WChar> CharSet;

	struct GlyphInfo;

	struct Glyph;

	struct Face;

	struct FontImpl;

	template <System::ImageFormat FORMAT> struct FontImplOfType;

	struct Buffer;


	Freetype();

	~Freetype();

	FT_Open_Args * MakeOpenArgs(const Data::Archive::View & ttf);

	Buffer & AquireBuffer(UInt size);

	void ReleaseBuffer(Buffer & buffer);

	static Float GetAdvance(Face & face);

	static Float GetAscension(Face & face);

	static Float GetDescension(Face & face);


	FT_Library m_ftlibrary;

	FT_Open_Args m_openargs;


	struct Buffer :
		public Reflex::Item <Buffer>,
		public Array <Pair<System::fRect>>
	{
		typedef Reflex::Item <Buffer> Item;

		using Item::Attach;
		using Item::Detach;
	};

	Buffer::List m_free;


	static constexpr Key32 kRenderingModes[] = { "", K32("smooth"), K32("crisp"), K32("lcd"), K32("lcd-v") };

	static const CharSet kWestern, kJapanesePunctuation, kJapaneseHiragana, kJapaneseKatakana, kJapaneseKanji, kKoreanHangul;
};

struct Freetype::FontImpl : public GLX::Detail::Font
{
	static const FunctionPointer <FontImpl*()> kCreateFns[2];

	FontImpl(System::ImageFormat format);

	void Init(const ArrayView <FaceDesc> & faces);


	Pair <Float32> GetHeightAndTail() const override;

	Float32 GetTextWidth(const WString::View & text) const override;

	Pair <TRef <Graphic>,Float32> CreateText(const WString::View & text, Point position, Size scale) const override;


	virtual Data::Archive::View AllocateAndClearBuffer(System::iSize size, Int pixeldensity) const = 0;

	virtual void Render(const FT_Bitmap & ftbitmap, const Glyph & glyph, UInt w, UInt h) = 0;


	void AddFace(const FaceDesc & face);

	void Accommodate(Float asc, Float desc);


	Glyph & GetGlyph(WChar index, WChar character) const;

	REFLEX_INLINE Glyph & GetGlyph(WChar character) const { return GetGlyph(character, character); }


	bool AquireSlot(UInt width, Pair <UInt> & slot) const;

	void Consolidate() const;

	void ExpandBitmap() const;

	void CommitBitmap() const;


	Reference <Freetype> m_freetype;

	Int32 m_mode;

	Size m_size;

	Array < Reference <Face> > m_faces;

	Array <CharSet> m_charsets;

	Glyph * m_tab;

	Glyph * m_missing;


	Pair <Float32> m_height_and_tail;

	UInt m_max_height;


	mutable Reference <System::Renderer::Canvas> m_bitmap;

	mutable System::BitmapInfo m_bitmap_info;

	mutable bool m_needs_commit;

	mutable bool m_needs_notify;


	mutable Sequence <WChar,Glyph> m_glyphs;

	mutable Sequence <UInt,UInt> m_free_slots;


	mutable Data::Archive::View m_pbitmap_view;
};

template <System::ImageFormat FORMAT>
struct Freetype::FontImplOfType : public FontImpl
{
	REFLEX_STATIC_ASSERT(FORMAT == System::kImageFormatRGBA || FORMAT == System::kImageFormatLuminance);

	typedef ConditionalType <FORMAT == System::kImageFormatRGBA,UInt32,UInt8> PixelType;


	FontImplOfType();

	virtual Data::Archive::View AllocateAndClearBuffer(System::iSize size, Int pixdensity) const override;

	virtual void Render(const FT_Bitmap & ftbitmap, const Glyph & glyph, UInt w, UInt h) override;


	mutable Array <PixelType> m_bitmap_buffer;
};

struct Freetype::Face : public Reflex::Object
{
	Face(FontImpl & font, Float yoffset, Float spacing)
		: font(font),
		ascension(0.0f),
		descension(0.0f),
		yoffset(yoffset),
		spacing(spacing)
	{
	}

	~Face()
	{
		FT_Done_Face(ftface);
	}

	FontImpl & font;

	ConstReference <Data::ArchiveObject> fontfile;

	FT_Face ftface;

	Float ascension, descension;

	Float yoffset;

	Float spacing;
};

struct Freetype::GlyphInfo
{
	GlyphInfo(Face & face, WChar character, Int32 mode);

	GlyphInfo(const Glyph & glyph, Int32 mode);	//rebuild case

	~GlyphInfo();


	FT_BitmapGlyph GetBitmapGlyph();

	Glyph & Render(const FT_BitmapGlyph & ftbitmapglyph, Glyph & glyph);

	inline explicit operator bool() const { return ftglyph; }


	Face & face;

	WChar character;

	Float advance;

	FT_Glyph ftglyph;
};

struct Freetype::Glyph
{
	Glyph(Face & face, WChar character = 0);

	Face & face;

	WChar character;

	Float32 advance;

	Pair <UInt> slot;	//in raw pixels

	System::fRect src, dst;
};

const FunctionPointer <Freetype::FontImpl*()> Freetype::FontImpl::kCreateFns[2] =
{
	[]() -> FontImpl*
	{
		return REFLEX_CREATE(FontImplOfType<System::kImageFormatRGBA>);
	},
	[]() -> FontImpl*
	{
		return REFLEX_CREATE(FontImplOfType<System::kImageFormatLuminance>);
	}
};

const Freetype::CharSet Freetype::kWestern = { 32, 96, 'A', 'y' };
const Freetype::CharSet Freetype::kJapanesePunctuation = { 12288, 32, 12308, 12317 };
const Freetype::CharSet Freetype::kJapaneseHiragana = { 12353, 95, 12363, 12441 };
const Freetype::CharSet Freetype::kJapaneseKatakana = { 12449, 95, 12460, 12539 };
const Freetype::CharSet Freetype::kJapaneseKanji = { 19968, 40869 - 19968, 19982, 19968 };
const Freetype::CharSet Freetype::kKoreanHangul = { 4352, 24, 4352, 4352 };

Freetype::Freetype()
{
	FT_Init_FreeType(&m_ftlibrary);

	MemClear(&m_openargs, sizeof(m_openargs));

	m_openargs.flags = FT_OPEN_MEMORY;
}

Freetype::~Freetype()
{
	FT_Done_FreeType(m_ftlibrary);
}

REFLEX_INLINE FT_Open_Args * Freetype::MakeOpenArgs(const Data::Archive::View & ttf)
{
	FT_Open_Args & openargs = m_openargs;

	openargs.memory_base = ttf.data;
	openargs.memory_size = ttf.size;

	return &openargs;
}

REFLEX_INLINE Freetype::Buffer & Freetype::AquireBuffer(UInt size)
{
	auto buffer = m_free.GetLast();

	if (!buffer) buffer = REFLEX_CREATE(Buffer);

	Retain(*buffer);

	buffer->SetSize(size);

	return *buffer;
}

REFLEX_INLINE void Freetype::ReleaseBuffer(Buffer & buffer)
{
	buffer.Attach(m_free);

	Release(buffer);
}

REFLEX_INLINE Float Freetype::GetAdvance(Face & face)
{
	FT_Glyph_Metrics & metrics = face.ftface->glyph->metrics;

	return Float(metrics.horiAdvance >> 6) * kPixelSize;
}

REFLEX_INLINE Float Freetype::GetAscension(Face & face)
{
	FT_Glyph_Metrics & metrics = face.ftface->glyph->metrics;

	return Float(metrics.horiBearingY >> 6) * kPixelSize;
}

REFLEX_INLINE Float Freetype::GetDescension(Face & face)
{
	FT_Glyph_Metrics & metrics = face.ftface->glyph->metrics;

	return Float((metrics.height - metrics.horiBearingY) >> 6) * kPixelSize;
}

Freetype::GlyphInfo::GlyphInfo(Face & face, WChar character, Int32 mode)
	: face(face)
	, character(character)
	, advance(face.spacing)
	, ftglyph(0)
{
	REFLEX_ASSERT(character);

	auto ftface = face.ftface;

	if (auto idx = FT_Get_Char_Index(ftface, character))
	{
		FT_Load_Glyph(ftface, idx, mode);

		FT_Get_Glyph(ftface->glyph, &ftglyph);

		advance += GetAdvance(face);
	}
}

REFLEX_INLINE Freetype::GlyphInfo::GlyphInfo(const Glyph & glyph, Int32 mode)
	: face(glyph.face)
	, character(glyph.character)
	, advance(glyph.advance)
	, ftglyph(0)
{
	REFLEX_ASSERT(character);

	auto ftface = face.ftface;

	if (auto idx = FT_Get_Char_Index(ftface, character))
	{
		FT_Load_Glyph(ftface, idx, mode);

		FT_Get_Glyph(ftface->glyph, &ftglyph);
	}
}

REFLEX_INLINE Freetype::GlyphInfo::~GlyphInfo()
{
	FT_Done_Glyph(ftglyph);
}

REFLEX_INLINE FT_BitmapGlyph Freetype::GlyphInfo::GetBitmapGlyph()
{
	FT_Glyph_To_Bitmap(&ftglyph, FT_RENDER_MODE_NORMAL, 0, 1);

	return reinterpret_cast<FT_BitmapGlyph>(ftglyph);
}

Freetype::Glyph & Freetype::GlyphInfo::Render(const FT_BitmapGlyph & ftbitmapglyph, Glyph & g)
{
	auto library = g_library;

	auto & font = g.face.font;


	auto pixdensity = library->m_pixeldensity;

	REFLEX_ASSERT(pixdensity == UInt32(font.m_bitmap_info.pixdensity));


	auto & ftbitmap = ftbitmapglyph->bitmap;

	UInt w = ftbitmap.width;

	REFLEX_ASSERT(g.slot.b == w + (pixdensity * 2));

	UInt h = Reflex::Min<UInt>(ftbitmap.rows, font.m_max_height * pixdensity);

	REFLEX_ASSERT(UInt(ftbitmap.rows) <= h);

	g.advance = advance;


	font.Render(ftbitmap, g, w, h);

	Float fpixdens = library->m_fdpifactor;

	Float fpixrcp = Reflex::Reciprocal(fpixdens);

	Float left = Float(ftbitmapglyph->left) * fpixrcp;

	Float top = Float(ftbitmapglyph->top) * fpixrcp;

	g.src = { { (Float(g.slot.a) * fpixrcp) + 1.0f, 1.0f}, { Float(w) * fpixrcp, Float(h) * fpixrcp } };

	g.dst.origin = { left, -top + g.face.yoffset };

	g.dst.size = g.src.size;

	return g;
}

Freetype::FontImpl::FontImpl(System::ImageFormat imageformat)
	: m_freetype(The<Freetype>::Acquire()),
	m_tab(nullptr),
	m_missing(nullptr),
	m_max_height(0),
	m_needs_commit(false),
	m_needs_notify(false)
{
	m_bitmap_info.format = imageformat;
	m_bitmap_info.size.w = 0;
	m_bitmap_info.size.h = 0;
	m_bitmap_info.pixdensity = g_library->m_pixeldensity;
}

void Freetype::FontImpl::Init(const ArrayView <FaceDesc> & faces)
{
	if (faces.size > 1)
	{
		m_charsets.Allocate(6);

		m_charsets.Push(kKoreanHangul);
		m_charsets.Push(kJapaneseKanji);
		m_charsets.Push(kJapaneseKatakana);
		m_charsets.Push(kJapaneseHiragana);
		m_charsets.Push(kJapanesePunctuation);
	}

	m_charsets.Push(kWestern);

	for (auto & i : faces) AddFace(i);

	if (m_faces)
	{
		auto & first = *m_faces.GetFirst();

		m_height_and_tail = { first.ascension, first.descension };

		constexpr auto BuildMissing = [](FontImpl & self)
		{
			constexpr WChar missing[] = { 65533, 9647, 9633, L'?' };

			for (auto character : missing)
			{
				for (auto & i : self.m_faces)
				{
					auto & face = *i;

					Freetype::GlyphInfo info(face, character, self.m_mode);

					if (info)
					{
						self.Accommodate(GetAscension(face), GetDescension(face));

						self.m_missing = &self.GetGlyph(character);

						return true;
					}
				}
			}

			return false;
		};

		if (!BuildMissing(*this))
		{
			auto & null = m_glyphs.Insert(L'?', Glyph(first, L'?'));

			null.advance = 4.0f;

			null.src.size = { 0.0f, 0.0f };
			null.dst.size = { 0.0f, 0.0f };

			m_missing = &null;//. Empty::Call(*this, first, L'?', L'?', 0.0f); //&null;
		}

		Freetype::GlyphInfo info(first, L' ', m_mode);

		Accommodate(GetAscension(first), GetDescension(first));

		m_tab = &GetGlyph(9, L' ');

		m_tab->advance *= 4.0f;
	}
}

void Freetype::FontImpl::AddFace(const FaceDesc & facedesc)
{
	auto & size = facedesc.size;

	m_size = Max(m_size, size);

	auto & face = *m_faces.Push(REFLEX_CREATE(Face, *this, facedesc.yoffset, facedesc.spacing));

	face.fontfile = facedesc.fontfile;

	auto & ftface = face.ftface;

	ftface = 0;

	if (FT_Open_Face(m_freetype->m_ftlibrary, m_freetype->MakeOpenArgs(face.fontfile->value), 0, &ftface))
	{
		m_mode = FT_LOAD_TARGET_NORMAL;

		m_faces.Pop();
	}
	else
	{
		switch (facedesc.mode)
		{
		default:
			m_mode = FT_LOAD_TARGET_NORMAL;
			break;

		case kRenderSmooth:
			m_mode = FT_LOAD_TARGET_LIGHT;
			break;

		case kRenderCrisp:
			m_mode = FT_LOAD_TARGET_MONO;
			break;

		case kRenderLCD_H:
			m_mode = FT_LOAD_TARGET_LCD;
			break;

		case kRenderLCD_V:
			m_mode = FT_LOAD_TARGET_LCD_V;
			break;
		}

		m_mode |= (facedesc.antialias ? FT_LOAD_FORCE_AUTOHINT : FT_LOAD_NO_AUTOHINT);


		auto dpi = 72 * g_library->m_pixeldensity;

		FT_Set_Char_Size(ftface, ToInt32(size.w * 64.0f), ToInt32(size.h * 64.0f), dpi, dpi);



		//get sizes

		auto & ascension = face.ascension;

		auto & descension = face.descension;

		REFLEX_RLOOP(charsetidx, m_charsets.GetSize())
		{
			auto & charset = m_charsets[charsetidx];

			GlyphInfo max(face, charset.c, m_mode);

			if (max) ascension = Reflex::Max(ascension, Freetype::GetAscension(face));

			GlyphInfo min(face, charset.d, m_mode);

			if (min) descension = Reflex::Max(descension, Freetype::GetDescension(face));

			if (max && min)
			{
				Accommodate(ascension, descension);

				m_charsets.Remove(charsetidx);
			}
		}
	}
}

Pair <Float32> Freetype::FontImpl::GetHeightAndTail() const
{
	return m_height_and_tail;
}

Float32 Freetype::FontImpl::GetTextWidth(const WString::View & text) const
{
	REFLEX_ASSERT(m_faces);

	Float32 width = 0.0f;

	for (auto character : text)
	{
		width += RemoveConst(this)->GetGlyph(character).advance;
	}

	return width;
}

Pair < TRef <Graphic>, Float32 > Freetype::FontImpl::CreateText(const WString::View & value, Point position, Size scale) const
{
	REFLEX_ASSERT(m_faces);

	if (value)
	{
		auto & ft = *m_freetype;

		auto & buffer = ft.AquireBuffer(value.size);

		auto prects = buffer.GetData();

		Float width = 0.0f;

		REFLEX_LOOP(idx, value.size)
		{
			auto & glyph = GetGlyph(value[idx]);

			auto & rects = *prects++;

			rects.a = glyph.src;

			auto & dst = rects.b;

			dst = glyph.dst;

			dst.origin.x = ((dst.origin.x + width) * scale.w) + position.x;

			dst.origin.y = (dst.origin.y * scale.h) + position.y;

			dst.size.w *= scale.w;

			dst.size.h *= scale.h;

			width += glyph.advance;
		}

		CommitBitmap();

		auto graphic = m_bitmap->CreateTextures(buffer);

		ft.ReleaseBuffer(buffer);

		return { graphic, width };//text;
	}
	else
	{
		return { gEmptyStringRenderer, 0.0f };
	}
}

void Freetype::FontImpl::Consolidate() const
{
	auto & size = m_bitmap_info.size;

	auto pixdensity = m_bitmap_info.pixdensity;

	UInt w = UInt(size.w * pixdensity);

	UInt x = 0;

	m_free_slots.Clear();

	if (m_glyphs)
	{
		REFLEX_RLOOP(idx, m_glyphs.GetSize())
		{
			auto & glyph = m_glyphs[idx].value;

			GlyphInfo info(glyph, m_mode);

			auto ftglyph = info.GetBitmapGlyph();

			UInt slotw = ftglyph->bitmap.width + (pixdensity * 2);

			if ((x + slotw) <= w)
			{
				glyph.slot = { x, slotw };

				x += slotw;

				REFLEX_ASSERT(info);

				info.Render(ftglyph, glyph);
			}
			else
			{
				REFLEX_ASSERT(false);

				glyph.slot = { x, 0 };
			}
		}
	}

	m_free_slots.Insert(x, w - x);

	m_needs_commit = True(m_glyphs);
}

void Freetype::FontImpl::ExpandBitmap() const
{
	auto & size = m_bitmap_info.size;

	auto oldw = size.w;

	constexpr bool kTest = REFLEX_DEBUG && false;

	auto min = Max<UInt>(RoundUpPow2(Truncate(m_size.w) * (kTest ? 2 : 26)), kTest ? 64 : 512);

	size.w = Clip<UInt>(oldw * 2, min, 2048);

	if (size.w > oldw)
	{
		auto pixdensity = m_bitmap_info.pixdensity;

		m_bitmap = Core::g_renderer->CreateBitmap(true, false);

		m_bitmap->SetSize(size, pixdensity);

		m_pbitmap_view = AllocateAndClearBuffer(size, pixdensity);

		Consolidate();
	}
}

void Freetype::FontImpl::CommitBitmap() const
{
	if (SetFiltered(m_needs_commit, false))
	{
#if (REFLEX_DEBUG)
		UInt pixels = m_bitmap_info.size.w * m_bitmap_info.size.h * Square(m_bitmap_info.pixdensity);

		REFLEX_ASSERT((m_pbitmap_view.size / System::kBPP[m_bitmap_info.format]) == pixels);
#endif
		m_bitmap->Write(m_bitmap_info.format, m_pbitmap_view);
	}
}

Freetype::Glyph & Freetype::FontImpl::GetGlyph(WChar index, WChar character) const
{
	if (auto pglyph = m_glyphs.SearchValue(index))
	{
		//REFLEX_ASSERT(pglyph->character == character);

		return *pglyph;
	}

	for (auto & i : m_faces)
	{
		auto & face = *i;

		GlyphInfo info(face, character, m_mode);

		if (info)
		{
			Glyph glyph(face, character);

			auto ftglyph = info.GetBitmapGlyph();

			UInt w = ftglyph->bitmap.width;

			if (AquireSlot(w, glyph.slot))
			{
				return m_glyphs.Insert(index, info.Render(ftglyph, glyph));
			}
			else
			{
				ExpandBitmap();

				if (AquireSlot(w, glyph.slot))
				{
					return m_glyphs.Insert(index, info.Render(ftglyph, glyph));
				}
				else
				{
					return *m_missing;
				}
			}
		}
	}

	return *m_missing;
}

bool Freetype::FontImpl::AquireSlot(UInt width, Pair <UInt> & slot) const
{
	width += m_bitmap_info.pixdensity * 2;

	REFLEX_RFOREACH(free_slot, m_free_slots)
	{
		//auto & free_slot = m_free_slots[idx];

		UInt length = free_slot.value;

		if (length >= width)
		{
			free_slot.value = length - width;

			slot = { free_slot.key + free_slot.value, width };

			m_needs_commit = true;

			return true;
		}
	}

	return false;
}

void Freetype::FontImpl::Accommodate(Float ascension, Float descension)
{
	m_max_height = Reflex::Max<UInt>(Truncate(Reflex::RoundUp(ascension + descension)) + 1, m_max_height);

	m_bitmap_info.size.h = RoundUpPow2(m_max_height + 2);

	m_max_height = (m_bitmap_info.size.h - 2);
}

template <System::ImageFormat FORMAT> Freetype::FontImplOfType<FORMAT>::FontImplOfType()
	: FontImpl(FORMAT)
{
}

template <System::ImageFormat FORMAT> Data::Archive::View Freetype::FontImplOfType<FORMAT>::AllocateAndClearBuffer(System::iSize size, Int pixdensity) const
{
	m_bitmap_buffer.SetSize(size.w * size.h * Square(pixdensity));

	m_bitmap_buffer.Fill(0);

	if constexpr (FORMAT == System::kImageFormatRGBA)
	{
		return Data::Pack(m_bitmap_buffer);
	}
	else if constexpr (FORMAT == System::kImageFormatLuminance)
	{
		return m_bitmap_buffer;
	}
}

template <System::ImageFormat FORMAT> void Freetype::FontImplOfType<FORMAT>::Render(const FT_Bitmap & ftbitmap, const Glyph & glyph, UInt w, UInt h)
{
	auto pixdensity = m_bitmap_info.pixdensity;

	auto roww = m_bitmap_info.size.w * pixdensity;

	auto bitmap = m_bitmap_buffer.GetData() + (roww * pixdensity) + glyph.slot.a + pixdensity;

	REFLEX_LOOP(y, h)
	{
		auto read = ftbitmap.buffer + (w * y);

		auto write = RemoveConst(bitmap) + (roww * y);

		REFLEX_ASSERT(write + w <= m_bitmap_buffer.GetData() + m_bitmap_buffer.GetSize());

		if constexpr (FORMAT == System::kImageFormatRGBA)
		{
			REFLEX_LOOP(x, w)
			{
				auto luminance = read[x];

				Reinterpret<Quad<UInt8>>(write[x]) = { luminance, luminance, luminance, luminance };
			}
		}
		else if constexpr (FORMAT == System::kImageFormatLuminance)
		{
			REFLEX_LOOP(x, w) write[x] = read[x];
		}
	}
}

Freetype::Glyph::Glyph(Face & face, WChar character)
	: face(face),
	character(character),
	advance(0.0f),
	src(kNormalRect),
	dst(kNormalRect)
{
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Object> Reflex::GLX::Detail::DecodeFontFile(const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream)
{
	return REFLEX_CREATE(FontFile, File::ReadBytes(instream));
}

Reflex::TRef <Reflex::GLX::Detail::Font> Reflex::GLX::Detail::Font::Create(const ArrayView <FaceDesc> & faces)
{
	constexpr auto MakeUID = [](const ArrayView <FaceDesc> & faces)
	{
		UInt64 hash = kHashSeed;

		for (auto & i : faces)
		{
			auto address = ToUIntNative(i.fontfile.Adr());

			Reflex::Detail::IncrementHash(hash, address);
			Reflex::Detail::IncrementHash(hash, Reinterpret<UInt64>(i.size));
			Reflex::Detail::IncrementHash(hash, Reinterpret<UInt32>(MakeTuple(Int16(Truncate(i.spacing * 1024.0f)), i.mode, i.antialias)));
		}

		return hash;
	};

	auto uid = MakeUID(faces);

	auto & font_cache = g_library->cache.fonts;

	if (auto pfontref = font_cache.Search(uid))
	{
		return *pfontref;
	}
	else
	{
		bool luminance = kSupportsImageFormat[System::kImageFormatLuminance];

		auto font = Freetype::FontImpl::kCreateFns[luminance]();
		
		font->Init(faces);

		if (font->m_faces)
		{
			return *font_cache.Set(uid, font);
		}
		else
		{
			auto t = AutoRelease(font);

			return null;
		}
	}
}

Reflex::TRef <Reflex::GLX::Detail::Font> Reflex::GLX::Detail::Font::Create(const Data::PropertySet & properties)
{
	constexpr auto ParseFace = [](Array <FaceDesc> & faces, const Data::PropertySet & properties, Data::ArrayOfFloat32Property * & psizes)
	{
		if (auto path = GetPathProperty(properties))
		{
			auto & face = faces.Push();

			psizes = properties.QueryProperty<Data::ArrayOfFloat32Property>(ksize, psizes);

			face.fontfile = RetrieveFontFile(path);

			face.size = ToSize(psizes->value);
			face.yoffset = ToSize(Data::GetFloat32Array(properties, koffset)).h;
			face.spacing = GetNumber(properties, K32("spacing"));
			face.mode = ParseEnum(Data::GetKey32(properties, krender), Freetype::kRenderingModes, Font::kRenderDefault);
			face.antialias = Data::GetBool(properties, kantialias);
		}
	};

	if (auto path = GetPathProperty(properties))
	{
		auto & faces = g_library->cache.faces;

		faces.Clear();

		auto size = GLX::GetSize(properties, ksize, { 12.0f, 12.0f });

		Data::ArrayOfFloat32Property sizes = ToView({ size.w, size.h });

		auto psizes = &sizes;

		ParseFace(faces, properties, psizes);

		if (faces)
		{
			for (auto & i : Data::GetPropertySetArray(properties, K32("alt")))
			{
				ParseFace(faces, i, psizes);
			}
		}

		return Create(faces);
	}
	else
	{
		return null;
	}
}
