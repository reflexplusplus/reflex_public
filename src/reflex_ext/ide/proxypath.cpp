#include "globalimpl.h"




REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct ProxyPathImpl : public ProxyPath
{
	using ProxyPath::ProxyPath;

	WString Remap(const ArrayView <WString::View> & sub_domain, const WString::View & path) const
	{
		WString::View expand = WString::View(sub_domain.GetFirst().data, UInt(sub_domain.GetLast().data + sub_domain.GetLast().size - sub_domain.GetFirst().data));

		if (ProxyPath::sub_domain == Key32(expand))
		{
			return Join(ProxyPath::local_path, path);
		}

		return {};
	}

	virtual TRef <System::FileHandle> OnRead(const ArrayView <WString::View> & sub_domain, const WString::View & path, File::Attributes & attributes) const override
	{
		if (sub_domain)
		{
			auto test = Remap(sub_domain, path);

			if (auto file = File::Detail::Open(test, File::kdisk, attributes))
			{
				attributes.resolved_path = test;

				return file;
			}
		}

		return {};
	}

	virtual TRef <System::FileHandle> OnWrite(const ArrayView <WString::View> & sub_domain, const WString::View & path, bool append) const override
	{
		if (sub_domain)
		{
			if (auto test = Remap(sub_domain, path))
			{
				return System::FileHandle::Create(test, append ? System::FileHandle::kModeAppend : System::FileHandle::kModeOverwrite);
			}
		}

		return {};
	}
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::IDE::ProxyPath> Reflex::IDE::ProxyPath::Create(Key32 domain, Key32 sub_domain, const WString::View & local_path)
{
	return REFLEX_CREATE(ProxyPathImpl, domain, sub_domain, local_path);
}
