#include "sdk.h"
#include "window.h"

#include "../common/graphics/functions.cpp"
#include "../common/graphics/metal.mm"


REFLEX_BEGIN_INTERNAL(Reflex::System::iOS)

struct ScreenCanvasImpl : public Common::Metal::ScreenCanvas
{
	ScreenCanvasImpl(Common::Metal& renderer)
		: ScreenCanvas(renderer)
	{
		REFLEX_ASSERT(Window::st_viewcontroller);
		UIView* view = Window::st_viewcontroller.view;

		@try
		{
			auto screen_size = view.bounds.size;

			m_layer = [CAMetalLayer layer];
			m_layer.pixelFormat = Common::Metal::kColorPixelFormat;
			m_layer.framebufferOnly = YES;
			m_layer.frame = view.bounds;
			m_layer.drawableSize = screen_size;

			[view.layer addSublayer:m_layer];

			// HACK: for when SetCurrent is called before SetSize.
			// We need to supply a multisample texture that matches the size of the framebuffer, and before SetSize is called, it would still be 1x1.
			SetSize({ Truncate(screen_size.width), Truncate(screen_size.height) }, GetMaxPixelDensity());
		}
		@catch (NSException* ex)
		{
			NSLog(@"Failed to create Metal renderer: %@", ex);
		}
	}

	~ScreenCanvasImpl() override 
	{
		[m_layer removeFromSuperlayer];
	}

	CAMetalLayer* GetMetalLayer() override { return m_layer; }

	// MEMO: as part of a bug, clang might show that this function hides a non-virtual function from struct 'ScreenCanvas', but I've checked that it is not the case (if the function is virtual in the base class, then it remains virtual in all the hierarchy without the virtual specifier)
	bool SetSize(const Reflex::System::iSize& size, Reflex::Int32 dpifactor) override
	{
		if (!m_renderer->IsDpiAware()) dpifactor = 1;

		m_layer.frame = CGRectMake(0, 0, size.w, size.h);
		m_layer.drawableSize = CGSizeMake(size.w * dpifactor, size.h * dpifactor);

		return ScreenCanvas::SetSize(size, dpifactor);
	}

	CAMetalLayer * m_layer;
};

struct MetalForIos : public Common::Metal
{
	using Common::Metal::Metal;

	TRef<Canvas> CreateCanvas(void* systemWindowPtr) override 
	{ 
		return REFLEX_CREATE(ScreenCanvasImpl, *this);
	}
};

TRef <Renderer> CreateMetal(const Renderer::Config & config)
{
	return REFLEX_CREATE(iOS::MetalForIos, true, false, Common::GetValue(config, Renderer::kTX, true));
}

const Common::RendererFactory g_metal_factory(Common::Metal::kEngineName, &CreateMetal);

REFLEX_END_INTERNAL
