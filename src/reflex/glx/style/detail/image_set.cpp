#include "reflex/glx/detail/image_set.h"
#include "reflex/glx/detail/svg.h"
#include "../stylesheet.h"




//
//image

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

Pair <Rect> g_frects = { {}, Detail::kNormalRect };

REFLEX_END_INTERNAL

Reflex::GLX::Detail::ImageSet::ImageSet(ConstTRef <System::Renderer::Canvas> source_bitmap)
	: source_bitmap(source_bitmap)
{
	Retain(source_bitmap);
}

Reflex::GLX::Detail::ImageSet::~ImageSet()
{
	Release(source_bitmap);
}

void Reflex::GLX::Detail::ImageSet::AddFrame(Key32 id, ConstTRef <Graphic> graphic, Size size)
{
	m_frames.Push({ id, graphic, size });
}

void Reflex::GLX::Detail::ImageSet::AddFrame(Key32 id, const Rect & rect)
{
	g_frects.a = rect;

	AddFrame(id, source_bitmap->CreateTextures({ &g_frects, 1 }), rect.size);
}

void Reflex::GLX::Detail::SetImage(GLX::Object & object, Key32 id, ConstTRef <Graphic> graphic, Size content_size)
{
	auto image_set = New<ImageSet>(Null<System::Renderer::Canvas>());

	image_set->AddFrame(id, graphic, content_size);

	object.SetProperty(id, image_set);

	object.RebuildLayout();
}

void Reflex::GLX::Detail::SetImage(GLX::Object & object, Key32 id, ConstTRef <System::Renderer::Canvas> bitmap)
{
	auto image_set = New<ImageSet>(bitmap);

	auto isize = bitmap->GetSize();

	image_set->AddFrame(id, { kOrigin, { ToFloat32(isize.w), ToFloat32(isize.h) } });

	object.SetProperty(id, image_set);

	object.RebuildLayout();
}

void Reflex::GLX::Detail::ClearImage(GLX::Object & object, Key32 id)
{
	object.UnsetProperty<ImageSet>(id);

	object.RebuildLayout();
}

Reflex::ConstTRef <Reflex::GLX::Detail::ImageSet> Reflex::GLX::Detail::CreateImageSet(const Data::PropertySet & properties)
{
	if (auto path = GetPathProperty(properties))
	{
		UInt pixel_density = Max(1, Truncate(GetNumber(properties, kdensity)));

		bool antialias = GLX::Detail::GetBool(properties, kantialias);

		auto bitmap = RetrieveBitmap(path, pixel_density, antialias);

		auto size = bitmap->GetSize();

		REFLEX_ASSERT((size.w && size.h));

		auto image_set = New<ImageSet>(bitmap);

		if (size.w * size.h)
		{
			if (auto tile = Data::GetFloat32Array(properties, K32("tile")))
			{
				UInt frame_id = Data::GetKey32(properties, K32("base_frame_id"), K32("a")).value;

				Size tilesize = ToSize(tile);

				System::iSize itilesize = { Truncate(tilesize.w), Truncate(tilesize.h) };

				tilesize = { Float(itilesize.w), Float(itilesize.h) };

				UInt ncol = size.w / itilesize.w;

				UInt nrow = size.h / itilesize.h;

				Float py = 0.0f;

				REFLEX_LOOP(row, nrow)
				{
					Float px = 0.0f;

					REFLEX_LOOP(col, ncol)
					{
						image_set->AddFrame(frame_id++, { {px, py}, {tilesize.w, tilesize.h} });
						
						px += tilesize.w;
					}

					py += tilesize.h;
				}
			}
			else if (auto frames = Data::GetPropertySet(properties, K32("frames")))
			{
				for (auto & i : frames->Iterate<Data::ArrayOfFloat32Property>())
				{
					auto & value = i.value->value;

					if (value.GetSize() == 4)
					{
						auto & frect = *Reinterpret<System::fRect>(value.GetData());

						image_set->AddFrame(i.key.id, frect);
					}
				}
			}
		}

		if (image_set->GetFrames().Empty())
		{
			Rect rect = { kOrigin, {Float(size.w), Float(size.h)} };

			image_set->AddFrame(kNullKey, rect);
		}

		return image_set;
	}

	return {};
}

Reflex::ConstTRef <Reflex::GLX::Detail::ImageSet> Reflex::GLX::Detail::CreateImageSetFromSVG(const Data::PropertySet & properties)
{
	if (auto path = GetPathProperty(properties))
	{
		auto xml = RetrieveRelativeResource<Data::PropertySet>(path, Data::PropertySet::null, kDecodeSVG);

		if (auto svgs = InspectSVG(xml))
		{
			Size size_override;

			if (auto sizes = Data::GetFloat32Array(properties, ksize))
			{
				size_override = ToSize(sizes);
			}


			Colour colour_override;

			for (auto id : kColourPropertyKeys)
			{
				if (auto pcolour = properties.QueryProperty<GLX::ColourProperty>(id))
				{
					colour_override = pcolour->value;

					break;
				}
				else if (auto pvalues = properties.QueryProperty<Data::ArrayOfFloat32Property>(id))	//TODO Style should always convert colour/bg_colour to ColourProperty
				{
					colour_override = ToColour(pvalues->value);

					break;
				}
			}


			auto image_set = New<ImageSet>(System::Renderer::Canvas::null);

			ColourPoints buffer;

			for (auto & i : svgs)
			{
				buffer.Clear();

				DecodeSVG(buffer, i, kNormal);

				if (colour_override.a)
				{
					for (auto & [point, colour] : buffer)
					{
						colour = colour_override;
					}
				}

				image_set->AddFrame(i.id, CreateGraphic(buffer), size_override.w ? size_override : i.size);
			}

			return image_set;
		}
	}

	return {};
}

