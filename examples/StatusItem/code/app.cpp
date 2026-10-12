#include "app.h"
#include "resources.h"




//
//StatusItemExample::App implementation

namespace StatusItemExample { namespace {

struct AppImpl : public App
{
	static constexpr UInt16 kChunkVersion = 0;

	AppImpl()
		: App(MakeKey32("StatusItem"), kChunkVersion)
		, m_status_item(New<System::StatusItem>())
	{
		if (!m_status_item)
		{
			output.Log("no status area on this platform, quitting");

			System::App::Quit();

			return;
		}

		m_status_item->SetTooltip(L"Reflex Status Item example");

		RefreshMenu();

		const auto preferred = System::StatusItem::GetPreferredBitmapInfo();

		auto encoded = File::Extract(::StatusItem::tray_icon_png);
		auto bitmap = GLX::Detail::DecodePNG(encoded);
		GLX::Detail::ConvertPixelDensity(bitmap.a, preferred.pixel_density);

		if (!m_status_item->SetBitmap(bitmap))
		{
			output.Log("could not publish status item bitmap, quitting");

			System::App::Quit();
		}
	}

	void RefreshMenu()
	{
		m_status_item->SetMenu
		({
			{ Join(L"Selected ", ToWString(m_count), L" times"), {} },
			{ L"" },
			{ L"Increment", [this]()
				{
					++m_count;

					RefreshMenu();
				}
			},
			{ L"" },
			{ L"Quit", []() { System::App::Quit(); } },
		});
	}



	//Data::iSerializable callbacks

	void OnReset(Key32 context) override
	{
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
	}

	bool OnImport(UInt16 version, Data::Archive::View & stream, Key32 context) override
	{
		return false;
	}

	void OnStore(Data::Archive & stream) const override
	{
	}

	Reference <System::StatusItem> m_status_item;
	UInt32 m_count = 0;
};

} }

Reflex::Unretained <StatusItemExample::App> StatusItemExample::App::Create()
{
	return New<StatusItemExample::AppImpl>();
}

Reflex::Output StatusItemExample::output("Status Item");
