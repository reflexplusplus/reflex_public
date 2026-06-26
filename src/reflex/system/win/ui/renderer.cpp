#include "dpi_awareness.h"
#include "../library.h"




bool Reflex::System::Win::InstantiateDpiAwareness(const System::Renderer::Config & config)
{
	globals->m_maxpixeldensity = 1;

	bool hd = Common::GetValue(config, Renderer::kHD, true);

	if (hd)
	{
		if (Library::st_hinstance) SetProcessDPIAware();

		HWND hwnd = GetDesktopWindow();

		RECT rect;

		GetClientRect(hwnd, &rect);

		if (rect.right > 1920 && rect.bottom > 1080)
		{
			HDC screen = GetDC(hwnd);

			int dpi = GetDeviceCaps(screen, LOGPIXELSX);

			globals->m_maxpixeldensity = Truncate(RoundUp(Float32(dpi) / 96.0f));

			ReleaseDC(hwnd, screen);
		}
	}

	return hd;
}

//Reflex::Array <Reflex::Pair <Reflex::CString::View, Reflex::System::Renderer::EngineCtr> > Reflex::System::Renderer::GetEngines() 
//{
//	Common::RendererFactory
//	return 
//	{
//		{ 
//			Win::Direct3D::kEngineName, 
//			[](const Config & config)
//			{
//				return Win::InstantiateRenderer(config, Win::CreateDirect3D);
//			}
//		},
//		{ 
//			Common::OpenGL::kEngineName, 
//			[](const Config & config)
//			{
//				return Win::InstantiateRenderer(config, Win::CreateWGL);
//			}
//		},
//	};
//}
