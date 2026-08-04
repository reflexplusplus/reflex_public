#include "[require].h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct DynamicLibraryImpl : public DynamicLibrary
{
	DynamicLibraryImpl(const WString & filename);

	~DynamicLibraryImpl();

	bool Status() const override;

	void * GetBundleRef() const override { return nullptr; }

	void * GetFunction(const CString & function) const override;

	HINSTANCE m_hlib;
};

DynamicLibraryImpl::DynamicLibraryImpl(const WString & filename)
{
	m_hlib = LoadLibraryW(filename.GetData());
}

DynamicLibraryImpl::~DynamicLibraryImpl()
{
	FreeLibrary(m_hlib);
}

bool DynamicLibraryImpl::Status() const
{
	return m_hlib != 0;
}

void * DynamicLibraryImpl::GetFunction(const CString & name) const
{
	auto p = GetProcAddress(m_hlib, name.GetData());

	return p;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::DynamicLibrary> Reflex::System::DynamicLibrary::Create(const WString & filename)
{
	return REFLEX_CREATE(Win::DynamicLibraryImpl, filename);
}

Reflex::TRef <Reflex::System::DynamicLibrary> Reflex::System::DynamicLibrary::CreateFromBundle(const WString & filename, bool)
{
	REFLEX_ASSERT(false);

	return DynamicLibrary::null;
}
