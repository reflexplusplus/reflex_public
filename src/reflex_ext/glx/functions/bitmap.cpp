#include "../../../../include/reflex_ext/glx/functions/bitmap.h"




//
//impl

Reflex::System::RawBitmap Reflex::GLX::CropBitmap(const System::BitmapInfo & info, Data::Archive::View data, UInt x, UInt y, UInt w, UInt h)
{
	auto pixel_density = info.pixel_density;
	UInt32 src_w = info.size.w * pixel_density;
	UInt32 src_h = info.size.h * pixel_density;

	System::RawBitmap rtn;

	rtn.a = info;

	rtn.a.size = MakeSize<Int32>(w, h);

	Detail::AllocateBitmap(rtn.a, rtn.b);

	UInt32 dst_x = x * pixel_density;
	UInt32 dst_y = y * pixel_density;
	UInt32 dst_w = w * pixel_density;
	UInt32 dst_h = h * pixel_density;

	if (Detail::VerifyBitmap(info, data))
	{
		if (And((dst_x + dst_w) <= src_w, (dst_y + dst_h) <= src_h))
		{
			UInt bpp = System::kBPP[info.format];

			UInt src_rowsize = src_w * bpp;

			UInt dest_rowsize = dst_w * bpp;

			const UInt8 * src_row = data.data + (dst_y * src_rowsize) + (dst_x * bpp);

			UInt8 * dest_row = rtn.b.GetData();

			REFLEX_LOOP(row, dst_h)
			{
				MemCopy(src_row, dest_row, dest_rowsize);

				src_row += src_rowsize;

				dest_row += dest_rowsize;
			}
		}
	}

	return rtn;
}

Reflex::System::RawBitmap Reflex::GLX::HalveBitmap(const System::BitmapInfo & info, Data::Archive::View data)
{
	System::RawBitmap rtn;

	rtn.a = info;

	if (Detail::VerifyBitmap(info, data))
	{
		Data::Archive & output = rtn.b;

		UInt nchn = System::kBPP[info.format];

		rtn.a.size.w /= 2;
		rtn.a.size.h /= 2;

		Detail::AllocateBitmap(rtn.a, output);

		UInt pixel_density = info.pixel_density;

		UInt pixw = rtn.a.size.w * pixel_density;
		UInt pixh = rtn.a.size.h * pixel_density;

		UInt rowsize = info.size.w * nchn * pixel_density;

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
	}
	else
	{
		rtn.a.size = { 0, 0 };
	}

	return rtn;
}

Reflex::Data::Archive Reflex::GLX::BilinearResizeBitmap(const System::BitmapInfo & source_info, ArrayView <UInt8> source, UInt w, UInt h)
{
	auto pixel_density = source_info.pixel_density;
	UInt source_width = source_info.size.w * pixel_density;
	UInt source_height = source_info.size.h * pixel_density;
	UInt destination_width = w * pixel_density;
	UInt destination_height = h * pixel_density;

	UInt nchn = System::kBPP[source_info.format];
	UInt source_stride = source_width * nchn;
	UInt destination_stride = destination_width * nchn;

	Data::Archive destination;

	if (Detail::VerifyBitmap(source_info, source) && w && h)
	{
		System::BitmapInfo dest_info;
		dest_info.format = source_info.format;
		dest_info.pixel_density = pixel_density;
		dest_info.size = MakeSize<Int32>(w, h);

		Detail::AllocateBitmap(dest_info, destination);

		auto source_data = source.data;
		auto destination_data = destination.GetData();
	
		Float source_x_multiplier = Float(source_width) / Float(destination_width);
		Float source_y_multiplier = Float(source_height) / Float(destination_height);

		REFLEX_LOOP(destination_y, destination_height)
		{
			Float source_y = ((Float(destination_y) + 0.5f) * source_y_multiplier) - 0.5f;
			source_y = Max(source_y, 0.0f);

			Int y0 = Truncate(source_y);
			Int y1 = y0 + 1;

			Float y_fraction = source_y - Float(y0);

			y0 = Max(0, Min(y0, Int(source_height) - 1));
			y1 = Max(0, Min(y1, Int(source_height) - 1));

			REFLEX_LOOP(destination_x, destination_width)
			{
				Float source_x = ((Float(destination_x) + 0.5f) * source_x_multiplier) - 0.5f;
				source_x = Max(source_x, 0.0f);

				Int x0 = Truncate(source_x);
				Int x1 = x0 + 1;

				Float x_fraction = source_x - Float(x0);

				x0 = Max(0, Min(x0, Int(source_width) - 1));
				x1 = Max(0, Min(x1, Int(source_width) - 1));

				const UInt8 * p00 = source_data + y0 * source_stride + x0 * nchn;
				const UInt8 * p10 = source_data + y0 * source_stride + x1 * nchn;
				const UInt8 * p01 = source_data + y1 * source_stride + x0 * nchn;
				const UInt8 * p11 = source_data + y1 * source_stride + x1 * nchn;

				UInt8 * destination_pixel = destination_data + destination_y * destination_stride + destination_x * nchn;

				REFLEX_LOOP(channel, nchn)
				{
					auto v00 = Float(p00[channel]);
					auto v01 = Float(p01[channel]);

					Float top = v00 + (Float(p10[channel]) - v00) * x_fraction;
					Float bottom = v01 + (Float(p11[channel]) - v01) * x_fraction;
					auto value = top + (bottom - top) * y_fraction;

					destination_pixel[channel] = UInt8(Clip<Int32>(Truncate(RoundNearest(value)), 0, 255));
				}
			}
		}
	}

	return destination;
}
