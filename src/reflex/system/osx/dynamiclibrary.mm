#include "sdk.h"

#include <dlfcn.h>




//
//component

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

struct DynamicLibraryImpl : public DynamicLibrary
{
	DynamicLibraryImpl(void * pdl);

	~DynamicLibraryImpl();

	bool Status() const override;

	void * GetBundleRef() const override { REFLEX_ASSERT(false); return 0; }

	void * GetFunction(const CString & function) const override;


	void * m_pdl;

	static constexpr UInt32 kOpenFlags[] =
	{
		RTLD_LOCAL | RTLD_LAZY,
		RTLD_GLOBAL | RTLD_LAZY,	//strange workaroubnd for REX library
	};
};

struct Bundle : public DynamicLibraryImpl
{
	Bundle(const Common::UTF8 & filename, void * pdl);

	~Bundle();

	void * GetBundleRef() const override { return m_bundleref; }

	mutable CFBundleRef m_bundleref;
};

DynamicLibraryImpl::DynamicLibraryImpl(void * pdl)
	: m_pdl(pdl)
{
}

DynamicLibraryImpl::~DynamicLibraryImpl()
{
	dlclose(m_pdl);
}

bool DynamicLibraryImpl::Status() const
{
	return true;
}

void * DynamicLibraryImpl::GetFunction(const CString & name) const
{
	return dlsym(m_pdl, name.GetData());
}

Bundle::Bundle(const Common::UTF8 & utf8, void * pdl)
	: DynamicLibraryImpl(pdl)
	, m_bundleref(0)
{
	if (CFURLRef url = CFURLCreateFromFileSystemRepresentation(0, Reinterpret<UInt8>(utf8.GetData()), (CFIndex)utf8.GetSize(), true))
	{
		//GetFunction("bundleEntry");

		m_bundleref = CFBundleCreate(kCFAllocatorDefault, url);

		if (m_bundleref)
		{
			CFErrorRef error = 0;

			if (!CFBundleLoadExecutableAndReturnError(m_bundleref, &error))
			{
				if (error)
				{
					//if (CFStringRef failureMessage = CFErrorCopyFailureReason (error))
					//{
					//	CFRelease (failureMessage);
					//}

					CFRelease(error);
				}

				CFRelease(m_bundleref);

				m_bundleref = 0;
			}
		}

		CFRelease(url);
	}
}

Bundle::~Bundle()
{
	if (m_bundleref)
	{
		if (CFBundleIsExecutableLoaded(m_bundleref)) CFBundleUnloadExecutable(m_bundleref);

		CFRelease(m_bundleref);
	}
}

template <bool BUNDLE> REFLEX_INLINE TRef <DynamicLibrary> CreateDynamicLibrary(const WString & filename, bool loadbundle)
{
	auto utf8_filename = Common::ToUTF8(filename);

	if (Common::POSIX::Exists(utf8_filename))
	{
		for (auto & i : Left(ToView(DynamicLibraryImpl::kOpenFlags),1))
		{
			if constexpr (BUNDLE)
			{
				auto dllpath = ResolveBundlePath(filename);

				if (void * pdl = dlopen(dllpath.GetData(), i))
				{
					if (loadbundle)
					{
						return REFLEX_CREATE(Bundle, utf8_filename, pdl);
					}
					else
					{
						return REFLEX_CREATE(DynamicLibraryImpl, pdl);
					}
				}
			}
			else if (void * pdl = dlopen(utf8_filename.GetData(), i))
			{
				return REFLEX_CREATE(DynamicLibraryImpl, pdl);
			}
		}
	}

	return DynamicLibrary::null;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::DynamicLibrary> Reflex::System::DynamicLibrary::Create(const WString & filename)
{
	return OSX::CreateDynamicLibrary<false>(filename, false);
}

Reflex::TRef <Reflex::System::DynamicLibrary> Reflex::System::DynamicLibrary::CreateFromBundle(const WString & filename, bool loadbundle)
{
	return OSX::CreateDynamicLibrary<true>(filename, loadbundle);
}
