#include "globalimpl.h"




REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct ProxyPathImpl : public ProxyPath
{
	using ProxyPath::ProxyPath;

	WString Remap(ArrayView <WString::View> subdomain, WString::View path) const
	{
		WString::View expand = WString::View(subdomain.GetFirst().data, UInt(subdomain.GetLast().data + subdomain.GetLast().size - subdomain.GetFirst().data));

		if (ProxyPath::subdomain == Key32(expand))
		{
			return Join(ProxyPath::local_path, path);
		}

		return {};
	}

	TRef <System::FileHandle> OnRead(ArrayView <WString::View> subdomain, WString::View path, File::Attributes & attributes) const override
	{
		if (subdomain)
		{
			auto test = Remap(subdomain, path);

			if (auto file = File::Detail::Open(test, File::kdisk, attributes))
			{
				attributes.resolved_path = test;

				return file;
			}
		}

		return {};
	}

	TRef <System::FileHandle> OnWrite(ArrayView <WString::View> subdomain, WString::View path, bool append) const override
	{
		if (subdomain)
		{
			if (auto test = Remap(subdomain, path))
			{
				return System::FileHandle::Create(test, append ? System::FileHandle::kModeAppend : System::FileHandle::kModeOverwrite);
			}
		}

		return {};
	}
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::IDE::ProxyPath> Reflex::IDE::ProxyPath::Create(Key32 domain, Key32 subdomain, WString::View local_path)
{
	return REFLEX_CREATE(ProxyPathImpl, domain, subdomain, local_path);
}
