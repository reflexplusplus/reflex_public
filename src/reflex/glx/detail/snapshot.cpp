#include "../../../../include/reflex/glx/detail/snapshot.h"
#include "transform_scope.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct AbstractCloneX : public AbstractClone
{
	struct Renderer : public Core::Renderer
	{
		Renderer(GLX::Object & owner)
			: Core::Renderer(false),
			m_owner(owner)
		{
		}

		TRef <Object> m_owner;
	};

	virtual void OnSetProperty(Address address, Reflex::Object & object) override
	{
		if (address == MakeAddress<Object>(K32("content")))
		{
			SetContent(Cast<Object>(object));
			
			return;
		}

		return Object::OnSetProperty(address, object);
	}
};

struct CloneView : public AbstractCloneX
{
	struct Renderer : public AbstractCloneX::Renderer
	{
		REFLEX_OBJECT(Renderer, Core::Renderer);

		using AbstractCloneX::Renderer::Renderer;

		virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override;
	};


	CloneView();

	virtual void SetContent(Object & source) override;

	virtual void OnClock(Float) override;

	static void OnAccommodate(CloneView & self, bool & /*isresponsive*/, Size & contentsize)
	{
		auto & source = *self.m_source;

		source.ComputeLayout();

		auto source_size = Max(source.contentsize, source.GetRect().size);

		self.m_scale = Reflex::MakeSize(source.GetComputedStyle()->GetScale());

		auto size = source_size / self.m_scale;

		contentsize = size * self.GetComputedStyle()->GetScale();	// / scale;

		contentsize.w = Reflex::RoundUp(contentsize.w);

		contentsize.h = Reflex::RoundUp(contentsize.h);
	}

	static void OnAlign(CloneView & self, bool /*isresponsive*/, Float & /*contenth*/)
	{
		self.m_position = self.GetRect().origin;
	}



	Core::WeakReference m_source;

	Size m_size_z;

	Float m_scale_z;

	Scale m_scale;

	Point m_position;
};

struct Snapshot : public AbstractCloneX
{
	struct Renderer : public AbstractCloneX::Renderer
	{
		REFLEX_OBJECT(Renderer, Core::Renderer);

		using AbstractCloneX::Renderer::Renderer;

		virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override;
	};


	Snapshot(bool antialias);

	virtual void SetContent(Object & source) override;

	static void OnAccommodate(Snapshot & object, bool & isresponsive, Size & contentsize);

	static void OnAlign(Snapshot & object, bool isresponsive, Float & contenth);


	Reference <System::Renderer::Canvas> m_bitmap;

	Reference <System::Renderer::Graphic> m_image;

	Size m_size;

	Scale m_scale;

	Colour m_white;
};

CloneView::CloneView()
	: m_scale_z(1.0f),
	m_scale(kNormal)
{
	struct LayoutModel : public Detail::LayoutModel
	{
		Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 flags) override
		{
			auto self = Cast<CloneView>(object);

			self->m_source->ComputeLayout();

			self->SetRenderer(New<Renderer>(self), self->m_source->GetComputedStyle()->GetZIndex());

			return CastLayoutFns<CloneView>(&CloneView::OnAccommodate, &CloneView::OnAlign);
		}
	};

	SetLayoutModel(New<LayoutModel>());

	EnableOnClock();
}

void CloneView::SetContent(Object & object)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	m_source = object;

	RebuildLayout();
}

void CloneView::OnClock(Float)
{
	auto & source = *m_source;

	source.ComputeLayout();

	if (Or(SetFiltered(m_size_z, source.contentsize), SetFiltered(m_scale_z, source.GetComputedStyle()->GetScale())))
	{
		Accommodate();
	}
	else
	{
		Redraw();	//for live redraw	//WAS Realign()
	}
}

void CloneView::Renderer::Render(const System::Renderer::Transform & transform, const Colour & colour) const
{
	auto self = Cast<CloneView>(m_owner);

	auto & source = *self->m_source;

	auto & ctx = *Core::RenderContext::st_current;

	TransformScope scope(ctx, self->m_position - source.GetRect().origin, self->m_scale);

	if (source.GetRenderer()) source.Draw(ctx);
}




//
//Snapshot (LEGACY)
 
//TODO remove
//add Clone() layer, that has Reference<Object> member, must be set in code
//create style with [Render(content: Clone())

Snapshot::Snapshot(bool antialias)
	: m_bitmap(Core::g_renderer->CreateBitmap(true, antialias))
	, m_scale(kNormal)
	, m_white(kWhite)
{
	struct LayoutModel : public Detail::LayoutModel
	{
		Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 flags) override
		{
			auto self = Cast<Snapshot>(object);

			self->SetRenderer(New<Renderer>(self), 0);

			return CastLayoutFns<Snapshot>(&Snapshot::OnAccommodate, &Snapshot::OnAlign);
		}
	};

	SetLayoutModel(New<LayoutModel>());
}

void Snapshot::SetContent(Object & object)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	auto object_rect = object.GetRect();

	m_size = Max(object_rect.size, Detail::ComputeContentSize(object));

	if (Detail::kCanRenderToTexture)
	{
		object.SetSize(m_size);

		object.Redraw();	//make sure the redraw flag is set

		auto isize = Max(Reflex::MakeSize(Truncate(m_size.w + Core::kRoundingTolerance), Truncate(m_size.h + Core::kRoundingTolerance)), System::Renderer::Canvas::kMinValidSize);

		if (auto ctx = Core::RenderContext::st_current)
		{
			m_bitmap->SetSize(isize, ctx->canvas->GetPixelDensity());

			RenderTargetScope scope(*ctx, *m_bitmap, kTransparent);

			ctx->transform.origin = -object_rect.origin;

			object.Draw(scope);
		}
		else
		{
			m_bitmap->SetSize(isize, System::GetMaxPixelDensity());

			m_bitmap->SetCurrent();

			Core::RenderContext temp = { .canvas = m_bitmap.Adr(), .viewport = {{0,0}, isize} };

			temp.clip_rect.Set(0, 0, isize.w, isize.h);

			Core::RenderContext::st_current = &temp;

			Core::g_renderer->SetClip(temp.viewport);

			Core::g_renderer->Clear(kTransparent);

			temp.transform.origin = -object_rect.origin;

			object.Draw(temp);
	
			m_bitmap->Flush();

			Core::RenderContext::st_current = 0;
		}

		Rect rect = { {}, m_size };

		Pair <System::fRect> rects = { rect, rect };

		m_image = m_bitmap->CreateTextures({ &rects, 1 });
	}

	RebuildLayout();
}

void Snapshot::OnAccommodate(Snapshot & self, bool & isresponsive, Size & contentsize)
{
	self.m_scale = Reflex::MakeSize(self.GetComputedStyle()->GetScale());

	contentsize = self.m_size * self.m_scale.w;

	contentsize.w = RoundDown<true>(contentsize.w + Core::kRoundingTolerance);

	contentsize.h = RoundDown<true>(contentsize.h + Core::kRoundingTolerance);
}

void Snapshot::OnAlign(Snapshot & self, bool isresponsive, Float & contenth)
{
	self.m_white.a = self.GetComputedStyle()->GetOpacity();
}

void Snapshot::Renderer::Render(const System::Renderer::Transform & transform, const Colour & colour) const
{
	auto self = Cast<Snapshot>(m_owner);

	Detail::Render(transform, *self->m_image, self->m_white, self->GetRect().origin, self->m_scale);
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Detail::AbstractClone> Reflex::GLX::Detail::CreateCloneView()
{
	return REFLEX_CREATE(CloneView);
}

Reflex::TRef <Reflex::GLX::Detail::AbstractClone> Reflex::GLX::Detail::CreateSnapshot(bool antialias)
{
	return REFLEX_CREATE(Snapshot, antialias);
}
