#include "graphics.h"

#include "../../common/graphics/metal.mm"




//
//macos metal impl

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

struct MetalImpl : public Common::Metal
{
	struct ScreenCanvasImpl : public Common::Metal::ScreenCanvas
	{
		ScreenCanvasImpl(Common::Metal & renderer, NSView *forView, id <MTLDevice> device)
			: ScreenCanvas(renderer)
		{
			m_layer = [CAMetalLayer layer];
			m_layer.device = device;
			m_layer.pixelFormat = Metal::kColorPixelFormat;
			m_layer.framebufferOnly = YES;
			m_layer.frame = CGRectZero;
			m_layer.drawableSize = { 1.0f, 1.0f };

			forView.wantsLayer = YES;
			forView.layer = m_layer;
		}

		~ScreenCanvasImpl()
		{
			[m_layer removeFromSuperlayer];
		}

		CAMetalLayer * GetMetalLayer() override { return m_layer; }

		bool SetSize(const iSize &size, Int32 dpifactor) override
		{
			if (!m_renderer->IsDpiAware()) dpifactor = 1;

			m_layer.frame = CGRectMake(0, 0, size.w, size.h);

			m_layer.drawableSize = CGSizeMake(size.w * dpifactor, size.h * dpifactor);

			m_layer.contentsScale = dpifactor;

			return ScreenCanvas::SetSize(size, dpifactor);
		}

		CAMetalLayer * m_layer;
	};

	using Common::Metal::Metal;

	TRef <Canvas> CreateCanvas(void * system_window) override
	{
		return REFLEX_CREATE(ScreenCanvasImpl, *this, (__bridge NSView*)system_window, GetMetalDevice());
	}
};

const Common::RendererFactory g_metal_factory(Common::Metal::kEngineName, &CreateRenderer<MetalImpl>);

REFLEX_END_INTERNAL
