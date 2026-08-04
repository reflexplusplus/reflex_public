#include "functions.h"





//
//defines

const Reflex::UInt8 Reflex::System::kBPP[System::kNumImageFormat] = { 3, 3, 4, 4, 1/*, 2*/ };

Reflex::Array <Reflex::Pair <Reflex::CString::View, Reflex::System::Renderer::EngineCtr> > Reflex::System::Renderer::GetEngines()
{
	Array <Pair <CString::View, Renderer::EngineCtr> > rtn;

	rtn.Allocate(3);

	for (auto & i : Common::RendererFactory::range)
	{
		rtn.Push<kAllocateNone>({ i.name, i.ctr });
	}

	return rtn;
}

Reflex::UInt Reflex::System::Common::GetValue(const Renderer::Config & config, Key32 key, UInt32 fallback)
{
	return *config.Search(key, &fallback);
}

bool Reflex::System::Common::VerifyBitmap(const BitmapInfo & info, const ArrayView <UInt8> & data)
{
	auto pixw = info.size.w * info.pixdensity;
	auto pixh = info.size.h * info.pixdensity;

	UInt32 bpp = kBPP[info.format];

	auto pix_area = pixw * pixh;

	auto is_expected_size = data.size == (pix_area * bpp);

	return pix_area && is_expected_size;
}
