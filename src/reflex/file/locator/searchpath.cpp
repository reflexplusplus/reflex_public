#include "../../../../include/reflex/file/virtual_filesystem/locators.h"
#include "../../../../include/reflex/file/functions.h"




//
//implementation

#define REFLEX_QUERY_MEMBER(object,id,adr,pobj) if (adr == MakeAddress<decltype(object)>(id)) { pobj = &object; return; }

REFLEX_BEGIN_INTERNAL(Reflex::File)

struct NullSearchPath : public SearchPath
{
	NullSearchPath() : SearchPath(kdisk, { kMaxUInt32, 0 }) {}
};

struct SearchPathImpl : public SearchPath
{
	REFLEX_OBJECT(File::SearchPathImpl, Locator);

	SearchPathImpl(const WString::View & path);

	virtual TRef <System::FileHandle> OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, Attributes & attributes) const override;

	virtual void OnQueryProperty(Address adr, Object * & ptr) const override
	{
		REFLEX_QUERY_MEMBER(m_path, K32("value"), adr, ptr);
	}

	mutable ObjectOf <WString> m_path;
};

SearchPathImpl::SearchPathImpl(const WString::View & path)
	: SearchPath(kdisk, { 0, 0 }),
	m_path(path)
{
	REFLEX_ASSERT(path);
}

TRef <System::FileHandle> SearchPathImpl::OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, Attributes & attributes) const
{
	WString test = Join(m_path.value, path);

	if (auto instream = Detail::Open(test, kdisk, attributes))
	{
		attributes.resolved_path = test;

		return instream;
	}

	Idx idx(path.size);

	while ((idx = ReverseSearch<CaseSensitive>(Left(path, idx.value), L'/')))
	{
		test = Join(m_path.value, Right(path, path.size - (idx.value + 1)));

		if (auto instream = Detail::Open(test, kdisk, attributes))
		{
			attributes.resolved_path = test;

			return instream;
		}

		if (!idx.value) break;

		idx.value--;
	}

	return {};
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::File::SearchPath> Reflex::File::SearchPath::Create(const WString::View & path)
{
	if (path)
	{
		return REFLEX_CREATE(SearchPathImpl, path);
	}
	else
	{
		REFLEX_ASSERT(false);

		return New<NullSearchPath>();
	}
}
