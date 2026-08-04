#include "bitmap.h"
#include "../../../library.h"
#include "../stylesheet.h"

#define LODEPNG_COMPILE_DECODER
#define LODEPNG_COMPILE_ENCODER

//#define LODEPNG_NO_COMPILE_ALLOCATORS
#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_CPP
#define LODEPNG_NO_COMPILE_ERROR_TEXT
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_CPP

REFLEX_DISABLE_WARNINGS
#define register
#include "../../ext/lodepng/lodepng.h"
#include "../../ext/lodepng/lodepng.cpp"
#include "../../ext/nanojpeg/nanojpeg.c"
REFLEX_ENABLE_WARNINGS
#include "../../ext/toojpeg/toojpeg.cpp"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct GLX_Header
{
	UInt32 magic;
	UInt16 version;
	UInt16 flags;
	UInt32 w, h;
	UInt8 format;
	UInt8 pixdensity;
};

constexpr UInt32 kGLX = 193456912ul;

constexpr UInt16 kBmpHeader = 19778;

void BMP_Unsupported(Data::Archive::View & stream, System::BitmapInfo & info, Data::Archive & data)
{
	info.format = System::kImageFormatRGBA;

	info.size = { 0, 0 };

	data.Clear();
}

void BMP_24bit(Data::Archive::View & stream, System::BitmapInfo & info, Data::Archive & data)
{
	info.format = System::kImageFormatBGR;

	UInt rowsize = info.size.w * 3;

	UInt rowpad = rowsize - ((rowsize / 4) * 4);

	if (rowpad)
	{
		UInt8 * dst = data.GetData();

		REFLEX_LOOP(idx, info.size.h)
		{
			MemCopy(stream.data, dst, rowsize);

			stream = Nudge(stream, rowsize + rowpad);

			dst += rowsize;
		}
	}
	else
	{
		data = Data::ReadBytes(stream, data.GetSize());
	}
}

void BMP_24bitInverted(Data::Archive::View & stream, System::BitmapInfo & info, Data::Archive & data)
{
	info.format = System::kImageFormatBGR;

	UInt rowsize = info.size.w * 3;

	UInt rowpad = rowsize - ((rowsize / 4) * 4);

	UInt8 * dst = data.GetData() + (rowsize * info.size.h);

	REFLEX_LOOP(idx, info.size.h)
	{
		dst -= rowsize;

		MemCopy(stream.data, dst, rowsize);

		stream = Nudge(stream, rowsize + rowpad);
	}
}

void BMP_32bit(Data::Archive::View & stream, System::BitmapInfo & info, Data::Archive & data)
{
	info.format = System::kImageFormatBGRA;

	UInt size = data.GetSize();

	MemCopy(stream.data, data.GetData(), size);

	stream = Nudge(stream, size);
}

void BMP_32bitInverted(Data::Archive::View & stream, System::BitmapInfo & info, Data::Archive & data)
{
	info.format = System::kImageFormatBGRA;

	UInt rowsize = info.size.w * 4;

	UInt8 * dst = data.GetData() + (rowsize * info.size.h);

	REFLEX_LOOP(idx, info.size.h)
	{
		dst -= rowsize;

		MemCopy(stream.data, dst, rowsize);

		stream = Nudge(stream, rowsize);
	}
}

template <UInt SIZE> void CopyBitmapImpl(const Data::Archive::Region & large_raw, System::iSize largesize, const Data::Archive::View & small_raw, System::iSize smallsize, System::iPoint pos)
{
	if (pos.x + smallsize.w <= largesize.w && pos.y + smallsize.h <= largesize.h)
	{
		if constexpr (SIZE == 1 || SIZE == 4)
		{
			auto large = Reinterpret<UInt32>(large_raw.data);

			auto small = Reinterpret<UInt32>(small_raw.data);

			REFLEX_LOOP(y, smallsize.h)
			{
				auto largerow = ((pos.y + y) * largesize.w) + pos.x;

				auto smallrow = (y * smallsize.w);

				REFLEX_LOOP(x, smallsize.w)
				{
					auto largeIndex = largerow + x;

					auto smallIndex = smallrow + x;

					large[largeIndex] = small[smallIndex];
				}
			}
		}
		else if constexpr (SIZE == 3)
		{
			auto large = large_raw.data;

			auto small = small_raw.data;

			REFLEX_LOOP(y, smallsize.h)
			{
				auto largerow = ((pos.y + y) * largesize.w) + pos.x;

				auto smallrow = (y * smallsize.w);

				REFLEX_LOOP(x, smallsize.w)
				{
					auto largeIndex = (largerow + x) * 3;

					auto smallIndex = (smallrow + x) * 3;

					*Reinterpret<UInt16>(&large[largeIndex]) = *Reinterpret<UInt16>(&small[smallIndex]);

					large[largeIndex + 2] = small[smallIndex + 2];
				}
			}
		}
	}
}

const decltype(&BMP_Unsupported) kLoadBMP[8] = { &BMP_Unsupported, &BMP_Unsupported, &BMP_24bit, &BMP_32bit, &BMP_Unsupported, &BMP_Unsupported, &BMP_24bitInverted, &BMP_32bitInverted };

REFLEX_END_INTERNAL




//
// private

const Reflex::FunctionPointer <void(Reflex::System::ImageFormat&, Reflex::Data::Archive&) > Reflex::GLX::Detail::kRemapBitmapFns[] =
{
	//RGB -> RGBA (all renderers must support RGBA)
	[](System::ImageFormat & format, Data::Archive & data)
	{
		auto npixel = data.GetSize() / 3;

		Data::Archive out(npixel * 4);

		auto rgb = data.GetData();

		auto end = rgb + (npixel * 3);

		auto rgba = out.GetData();

		while (rgb < end)
		{
			rgba[0] = rgb[0];
			rgba[1] = rgb[1];
			rgba[2] = rgb[2];
			rgba[3] = 255;

			rgb += 3;
			rgba += 4;
		}

		format = System::kImageFormatRGBA;

		data.Swap(out);
	},

	//BGR -> RGB
	[](System::ImageFormat & format, Data::Archive & data)
	{
		auto npixel = data.GetSize() / 3;

		auto ptr = data.GetData();

		auto end = ptr + (npixel * 3);

		while (ptr != end)
		{
			Reflex::Swap(ptr[0], ptr[2]);

			ptr += 3;
		}

		format = System::kImageFormatRGB;
	},

	//RGBA -> RGBA
	[](System::ImageFormat &, Data::Archive &) {},

	//BGRA -> RGBA
	[](System::ImageFormat & format, Data::Archive & data)
	{
		auto npixel = data.GetSize() / 4;

		auto ptr = data.GetData();

		auto end = ptr + (npixel * 4);

		while (ptr != end)
		{
			Reflex::Swap(ptr[0], ptr[2]);

			ptr += 4;
		}

		format = System::kImageFormatRGBA;
	},

	//lumiance
	[](System::ImageFormat &, Data::Archive &) 
	{
	}	//luminance (not needed as no image formats load this)
};




// 
//public

const Reflex::File::ResourcePool::Ctr Reflex::GLX::Detail::kDecodeBitmap = [](const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream) -> TRef <Reflex::Object>
{
	auto density = Data::GetUInt32(ctx.options, kdensity, 1);

	auto aa = Data::GetBool(ctx.options, kantialias);

	return RemoveConst(*OpenBitmap(File::ReadBytes(instream), density, aa));
};

Reflex::ConstTRef <Reflex::System::Renderer::Canvas> Reflex::GLX::Detail::OpenBitmap(const System::BitmapInfo & info, const Data::Archive::View & data, bool antialias)
{
	if (VerifyBitmap(info, data))
	{
		auto bitmap = Core::g_renderer->CreateBitmap(info.format > System::kImageFormatBGR, antialias);

		bitmap->SetSize(info.size, info.pixdensity);

		bitmap->Write(info.format, data);

		return bitmap;
	}
	else
	{
		if constexpr (REFLEX_DEBUG) output.Error("GLX::Detail::OpenBitmap invalid bitmap/bytes size");

		return System::Renderer::Canvas::null;
	}
}

Reflex::ConstTRef <Reflex::System::Renderer::Canvas> Reflex::GLX::Detail::RetrieveBitmap(const WString::View & path, UInt density, bool antialias)
{
	Data::PropertySet options;

	Data::SetUInt32(options, kdensity, density);
	
	Data::SetBool(options, kantialias, antialias);

	return RetrieveRelativeResource<System::Renderer::Canvas>(path, options, kDecodeBitmap);
}

Reflex::System::RawBitmap Reflex::GLX::Detail::DecodeBitmap(const ArrayView <UInt8> & data, UInt pixeldensity)
{
	System::RawBitmap rawbitmap;

	auto & bitmapinfo = rawbitmap.a;

	bitmapinfo.format = System::kImageFormatRGB;

	bitmapinfo.size = {};

	bitmapinfo.pixdensity = pixeldensity;

	if (data.size > 8)
	{
		UInt32 header = *Reinterpret<UInt32>(data.data);

		UInt16 h16 = Reinterpret<UInt16>(header);

		if (header == kPNG)
		{
			return DecodePNG(data, pixeldensity);
		}
		else if (h16 == kJPG)
		{
			return DecodeJPG(data, pixeldensity);
		}
		else if (header == kGLX)
		{
			return DecodeGLX(data);
		}
		else
		{
			return DecodeBMP(data, pixeldensity);
		}
	}

	return rawbitmap;
}

void Reflex::GLX::Detail::PreMultAlpha(const Reflex::System::BitmapInfo & info, Array<UInt8>& data)
{
	auto pdata = data.GetData();

	UInt32 bpp = System::kBPP[info.format];

	auto pend = data.GetData() + (info.size.w * info.pixdensity * info.size.h * info.pixdensity * bpp);

	REFLEX_ASSERT((pend - pdata) == data.GetSize());

	switch (info.format)
	{
	case System::kImageFormatRGBA:
	case System::kImageFormatBGRA:
	{
		while (pdata < pend)
		{
			UInt8 * px = pdata;
			UInt8 a = px[3];
			px[0] = UInt8((px[0] * a) / 255);
			px[1] = UInt8((px[1] * a) / 255);
			px[2] = UInt8((px[2] * a) / 255);
			pdata += 4;
		}
		break;
	}
	default:
		break;
	}
}

Reflex::Data::Archive Reflex::GLX::Detail::EncodePNG(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 compress)
{
	constexpr Pair <System::ImageFormat,LodePNGColorType> kFormats[] =
	{
		{ System::kImageFormatLuminance, LCT_GREY },
		{ System::kImageFormatRGB, LCT_RGB },
		{ System::kImageFormatRGBA, LCT_RGBA },
	};

	Data::Archive rtn;

	auto pformat = SearchValue<KeyCompare>(ToView(kFormats), info.format);

	if (pformat && VerifyBitmap(info, data))
	{
		Int32 w = info.size.w * info.pixdensity;

		Int32 h = info.size.h * info.pixdensity;

		UInt8 * output = 0;

		size_t lsize = 0;

		LodePNGState state;

		lodepng_state_init(&state);

		if (!compress)
		{
			state.encoder.filter_strategy = LFS_ZERO;
			state.encoder.zlibsettings.btype = 0;
			state.encoder.zlibsettings.use_lz77 = 0;
		}

		auto colortype = pformat->b;

		state.info_raw.colortype = colortype;
		state.info_raw.bitdepth = 8;
		state.info_png.color.colortype = colortype;
		state.info_png.color.bitdepth = 8;

		lodepng_encode(&output, &lsize, data.data, w, h, &state);

		lodepng_state_cleanup(&state);

		if (!state.error)
		{
			UInt size = UInt(lsize);

			rtn.SetSize(size);

			MemCopy(output, rtn.GetData(), size);

			lodepng_free(output);
		}
	}

	return rtn;
}

Reflex::System::RawBitmap Reflex::GLX::Detail::DecodePNG(const Data::Archive::View & input, UInt pixeldensity)
{
	System::RawBitmap rtn;

	auto & info = rtn.a;

	Data::Archive & output = rtn.b;

	info.format = System::kImageFormatRGBA;

	info.size = { 0,0 };

	info.pixdensity = pixeldensity;

	unsigned int w, h;

	LodePNGState state;
	lodepng_state_init(&state);

	if (!lodepng_inspect(&w, &h, &state, input.data, input.size))
	{
		state.info_raw.colortype = LCT_RGBA;
		state.info_raw.bitdepth = 8;

		if (state.info_png.color.colortype == LCT_GREY)
		{
			info.format = System::kImageFormatLuminance;
			state.info_raw.colortype = LCT_GREY;
		}

		UInt8 * pout = nullptr;

		if (!lodepng_decode(&pout, &w, &h, &state, input.data, input.size))
		{
			info.size = { Int(w / pixeldensity), Int(h / pixeldensity) };

			AllocateBitmap(info, output);
			MemCopy(pout, output.GetData(), output.GetSize());
			lodepng_free(pout);
		}
	}

	lodepng_state_cleanup(&state);

	return rtn;
}

Reflex::System::RawBitmap Reflex::GLX::Detail::DecodeJPG(const Data::Archive::View & input, UInt pixeldensity)
{
	System::RawBitmap rawbitmap;

	auto & info = rawbitmap.a;

	Data::Archive & data = rawbitmap.b;

	info.format = System::kImageFormatRGB;

	info.size = { 0, 0 };

	info.pixdensity = pixeldensity;

	nj_context_t nj;

	njInit(nj);

	if (njDecode(nj, input.data, input.size) == 0)
	{
		info.size.w = nj.width;

		info.size.h = nj.height;

		data.SetSize(njGetImageSize(nj));

		MemCopy(njGetImage(nj), data.GetData(), data.GetSize());

		REFLEX_ASSERT(data.GetSize() == UInt32(info.size.w * info.size.h * 3));

		info.size.w /= pixeldensity;

		info.size.h /= pixeldensity;
	}

	njDone(nj);

	RemapToSupportedFormat(info.format, data);

	return rawbitmap;
}

Reflex::Data::Archive Reflex::GLX::Detail::EncodeJPG(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 quality)
{
	Data::Archive rtn;

	if (VerifyBitmap(info, data) && (info.format == System::kImageFormatRGB))
	{
		TooJPEG::EncodeBitmap(data.data, info.size.w * info.pixdensity, info.size.h * info.pixdensity, 3, { quality }, &rtn, [](void * client, const UInt8 * data, UInt size)
		{
			Cast<Data::Archive>(client)->Append({ data, size });
		});
	}

	return rtn;
}

Reflex::System::RawBitmap Reflex::GLX::Detail::DecodeBMP(const Data::Archive::View & input, UInt pixeldensity)
{
	System::RawBitmap rawbitmap;

	auto & info = rawbitmap.a;

	Data::Archive & data = rawbitmap.b;

	info.size = { 0,0 };

	info.pixdensity = pixeldensity;

	auto stream = input;

	if (Data::Deserialize<UInt16>(stream) == kBmpHeader)
	{
		stream = Nudge(stream, 16);

		Pair <Int> pair;

		Data::Deserialize(stream, pair);

		Int & w = info.size.w;

		Int & h = info.size.h;

		w = pair.a;

		bool inverted;

		if (pair.b < 0)
		{
			inverted = false;

			h = -pair.b;
		}
		else
		{
			inverted = true;

			h = pair.b;
		}

		stream = Nudge(stream, 2);

		UInt channels = Data::Deserialize<UInt16>(stream) / 8;

		UInt num_bytes = w * h * channels;

		data.SetSize(num_bytes);

		UInt compression = Data::Deserialize<UInt32>(stream);

		stream = Nudge(stream, 20);

		if (compression)
		{
			BMP_Unsupported(stream, info, data);
		}
		else
		{
			UInt sel = (channels - 1) + (inverted * 4);

			(*kLoadBMP[sel])(stream, info, data);

			info.size = { Int(w / pixeldensity), Int(h / pixeldensity) };

			RemapToSupportedFormat(info.format, data);
		}
	}

	return rawbitmap;
}

Reflex::Data::Archive Reflex::GLX::Detail::EncodeBMP(const System::BitmapInfo & info, const Data::Archive::View & input, UInt8)
{
	struct BITMAPINFOHEADER
	{
		Int32 size;
		Int32 w;
		Int32 h;
		Int16 planes;
		Int16 bits;
		Int64 unused[3];
	};

	Data::Archive rtn;

	if (info.format == System::kImageFormatRGB)
	{
		Int32 w = info.size.w * info.pixdensity;
		Int32 h = info.size.h * info.pixdensity;

		BITMAPINFOHEADER bmpinfoheader = { sizeof(BITMAPINFOHEADER), w, -h, 1, 24 };

		REFLEX_LOOP(idx, 3) bmpinfoheader.unused[idx] = 0;

		rtn.Append(Data::Pack(kBmpHeader));
		rtn.Append(Data::Pack(UInt64(0)));
		rtn.Append(Data::Pack(UInt(54)));
		rtn.Append(Data::Pack(bmpinfoheader));

		auto rowsize = w * 3;

		auto padded = QuantiseUp(rowsize, 4);

		rtn.Allocate(rtn.GetSize() + (h * padded));

		auto src = input.data;

		REFLEX_LOOP(y, h)
		{
			auto dst = Extend<kAllocateNone>(rtn, padded);

			REFLEX_LOOP(x, w)
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];

				src += 3;
			}
		}

		*Reinterpret<UInt>(rtn.GetData() + 2) = rtn.GetSize();
	}

	return rtn;
}

Reflex::System::RawBitmap Reflex::GLX::Detail::DecodeGLX(const ArrayView <UInt8> & data)
{
	System::RawBitmap rawbitmap;

	auto & info = rawbitmap.a;

	if (data.size > sizeof(GLX_Header))
	{
		auto & header = *Reinterpret<GLX_Header>(data.data);

		if (header.magic == kGLX)
		{
			info.format = System::ImageFormat(header.format);

			info.pixdensity = header.pixdensity;

			info.size.w = header.w;

			info.size.h = header.h;

			Data::Archive & pixels = rawbitmap.b;

			Data::Archive::View chunk(data.data, data.size);

			chunk.data += sizeof(GLX_Header);

			chunk.size -= sizeof(GLX_Header);

			switch (header.flags)
			{
			case 2:
				pixels = Data::Decompress(Data::kLZ4, chunk);
				break;

			case 1:
				pixels = Data::Decompress(*gLZO, chunk);
				break;

			default:
				pixels = chunk;
				break;
			}

			RemapToSupportedFormat(info.format, pixels);

			return rawbitmap;
		}
	}

	info.format = System::kImageFormatRGBA;

	info.pixdensity = 1;

	info.size = { 0, 0 };

	return rawbitmap;
}

Reflex::Data::Archive Reflex::GLX::Detail::EncodeGLX(const System::BitmapInfo & info, const ArrayView <UInt8> & data, UInt8 compress)
{
	Data::Archive rtn;

	GLX_Header header = {kGLX, 0, UInt16(compress ? 2 : 0), UInt32(info.size.w), UInt32(info.size.h), UInt8(info.format), UInt8(info.pixdensity)};

	REFLEX_STATIC_ASSERT(sizeof(GLX_Header) == 20);

	rtn.Append(Data::Archive::View(Reinterpret<UInt8>(&header), sizeof(GLX_Header)));

	if (compress)
	{
		rtn.Append(Data::Compress(Data::kLZ4, data));
	}
	else
	{
		rtn.Append(data);
	}

	return rtn;
}

//void Reflex::GLX::Detail::CopyBitmap(const System::BitmapInfo & target_info, const Data::Archive::Region & target, System::iSize source_size, const Data::Archive::View & source, System::iPoint pos)
//{
//	decltype (&CopyBitmapImpl<0>) fns[] = { &CopyBitmapImpl<0>, &CopyBitmapImpl<0>, &CopyBitmapImpl<0>, &CopyBitmapImpl<3>, &CopyBitmapImpl<4> };
//
//	auto fn = fns[System::kBPP[target_info.format]];
//
//	System::iSize dest_size = { target_info.size.w * target_info.pixdensity, target_info.size.h * target_info.pixdensity };
//
//	System::iSize src_size = { source_size.w * target_info.pixdensity, source_size.h * target_info.pixdensity };
//
//	fn(target, dest_size, source, src_size, pos);
//}

Reflex::System::RawBitmap Reflex::GLX::Detail::CropBitmap(const System::BitmapInfo & info, const Data::Archive::View & data, const System::iRect & rect)
{
	Int32 src_w = info.size.w * info.pixdensity;

	Int32 src_h = info.size.h * info.pixdensity;

	Int32 x = rect.origin.x * info.pixdensity;

	Int32 y = rect.origin.y * info.pixdensity;

	Int32 w = rect.size.w * info.pixdensity;

	Int32 h = rect.size.h * info.pixdensity;

	System::RawBitmap rtn;

	rtn.a = info;

	rtn.a.size = rect.size;

	AllocateBitmap(rtn.a, rtn.b);

	if (VerifyBitmap(info, data))
	{
		if (And((x + w) < src_w, (y + h) < src_h))
		{
			UInt bpp = System::kBPP[info.format];

			UInt src_rowsize = src_w * bpp;

			UInt dest_rowsize = w * bpp;

			const UInt8 * src_row = data.data + (y * src_rowsize) + (x * bpp);

			UInt8 * dest_row = rtn.b.GetData();

			REFLEX_LOOP(row, h)
			{
				MemCopy(src_row, dest_row, dest_rowsize);

				src_row += src_rowsize;

				dest_row += dest_rowsize;
			}
		}
	}

	return rtn;
}

Reflex::System::RawBitmap Reflex::GLX::Detail::HalveBitmap(const System::BitmapInfo & info, const Data::Archive::View & data)
{
	System::RawBitmap rtn;

	rtn.a = info;

	if (VerifyBitmap(info, data))
	{
		Data::Archive & output = rtn.b;

		UInt nchn = System::kBPP[info.format];

		rtn.a.size.w /= 2;
		rtn.a.size.h /= 2;

		AllocateBitmap(rtn.a, output);

		UInt pixdensity = info.pixdensity;

		UInt pixw = rtn.a.size.w * pixdensity;

		UInt pixh = rtn.a.size.h * pixdensity;

		UInt rowsize = info.size.w * nchn * pixdensity;

		UInt halfrowsize = pixw * nchn;

		REFLEX_LOOP(y, pixh)
		{
			auto row = data.data + (rowsize * y * 2);

			auto destrow = output.GetData() + (halfrowsize * y);

			REFLEX_LOOP(x, pixw)
			{
				const UInt8 * rgb_0 = row + (x * nchn * 2);

				const UInt8 * rgb_1 = rgb_0 + nchn;

				const UInt8 * rgb_2 = rgb_0 + rowsize;

				const UInt8 * rgb_3 = rgb_2 + nchn;

				UInt8 * poutput = destrow + (x * nchn);

				REFLEX_LOOP(idx, nchn) poutput[idx] = UInt8((rgb_0[idx] + rgb_1[idx] + rgb_2[idx] + rgb_3[idx]) / 4);
			}
		}

		//rtn.a.size = { Int(pixw / pixdensity), Int(pixh / pixdensity) };
	}
	else
	{
		rtn.a.size = { 0, 0 };
	}

	return rtn;
}

//void Reflex::GLX::Detail::RemapBitmap(const System::BitmapInfo & info, const Data::Archive::Region & data, UInt32 word)
//{
//	if (VerifyBitmap(info, ToView(data)))
//	{
//		constexpr UInt32 MASK_2_BITS = 0x03; // 00000011 in binary
//
//		UInt32 maskedValue = (MASK_2_BITS << 0) | (MASK_2_BITS << 8) | (MASK_2_BITS << 16) | (MASK_2_BITS << 24);
//
//		word &= maskedValue;
//
//		auto order = Reinterpret<UInt8>(&word);
//
//		UInt order0 = order[0];
//		UInt order1 = order[1];
//		UInt order2 = order[2];
//		UInt order3 = order[3];
//
//		if (System::kBPP[info.format] == 4)
//		{
//			UInt8 t[4] = { 0,0,0,0 };
//
//			REFLEX_LOOP_PTR(Reinterpret<UInt32>(data.a), ptr, data.b / 4)
//			{
//				auto channels = Reinterpret<UInt8>(ptr);
//
//				t[0] = channels[order0];
//				t[1] = channels[order1];
//				t[2] = channels[order2];
//				t[3] = channels[order3];
//
//				*ptr = *Reinterpret<UInt32>(t);
//			}
//		}
//	}
//}

//Reflex::System::RawBitmap Reflex::GLX::Detail::FlipBitmap(const System::BitmapInfo & info, const Data::Archive::View & data, bool y)
//{
//	System::RawBitmap rtn;
//
//	rtn.a = info;
//
//	if (VerifyBitmap(info, data))
//	{
//		Data::Archive & output = rtn.b;
//
//		UInt nchn = System::kBPP[info.format];
//
//		AllocateBitmap(rtn.a, output);
//
//		UInt pixdensity = info.pixdensity;
//
//		//6UInt pixw = rtn.a.size.w * pixdensity;
//
//		UInt pixh = rtn.a.size.h * pixdensity;
//
//		UInt rowsize = info.size.w * nchn * pixdensity;
//
//		if (y)
//		{
//			auto size = pixh * rowsize;
//
//			const UInt8 * end = data.a + size;
//
//			UInt8 * dest_row = output.GetData() + size;
//
//			for (auto src_row = data.a; src_row < end; src_row += rowsize)
//			{
//				dest_row -= rowsize;
//
//				MemCopy(src_row, dest_row, rowsize);
//			}
//		}
//	}
//	else
//	{
//		rtn.a.size = { 0, 0 };
//	}
//
//	return rtn;
//}

REFLEX_NOINLINE void Reflex::GLX::Detail::AllocateBitmap(const System::BitmapInfo & info, Data::Archive & archive)
{
	archive.SetSize<kAllocateExact>(Square(info.pixdensity) * info.size.w * info.size.h * System::kBPP[info.format]);
}

REFLEX_NS(Reflex::System::Common)

bool VerifyBitmap(const BitmapInfo & info, const ArrayView <UInt8> & data);

REFLEX_END

bool(&Reflex::GLX::Detail::VerifyBitmap)(const Reflex::System::BitmapInfo & info, const Reflex::Data::Archive::View & data) = Reflex::System::Common::VerifyBitmap;
