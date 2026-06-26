#include "filesystem.h"




//
//implementation

Reflex::TRef <Reflex::File::VirtualFileSystem> Reflex::File::VirtualFileSystem::Create(Key32 defaultdomain, bool mt)
{
	return REFLEX_CREATE(FileSystemImpl, defaultdomain, mt);
}

Reflex::File::FileSystemImpl::FileSystemImpl(Key32 defaultdomain, bool mt)
	: m_defaultdomain(defaultdomain.value),
	m_cs(mt ? *System::CriticalSection::Create(true) : System::CriticalSection::null),
	m_lockcount(0)
{
}

Reflex::File::FileSystemImpl::~FileSystemImpl()
{
	Lock lock(*this);

	lock.Clear();
}

Reflex::File::VirtualFileSystem::Lock::Lock(VirtualFileSystem & filesystem)
	: System::CriticalSection::Lock(Cast<FileSystemImpl>(filesystem)->m_cs),
	filesystem(filesystem)
{
}

void Reflex::File::VirtualFileSystem::Lock::Clear()
{
	auto impl = Cast<FileSystemImpl>(filesystem);

	while (auto last = impl->m_list.GetLast()) Detach(*last);
}

void Reflex::File::VirtualFileSystem::Lock::Enumerate(const Function <void(Locator&)> & visitor)
{
	auto impl = Cast<FileSystemImpl>(filesystem);

	for (auto & i : impl->m_list)
	{
		visitor(i);
	}
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::VirtualFileSystem::Lock::Read(const WString::View & path, Attributes & attributes)
{
	auto impl = Cast<FileSystemImpl>(filesystem);

	attributes.resolved_path = path;
	attributes.status = Attributes::kStatusReady;

	impl->m_buffer.Clear();

	auto parts = Detail::SplitDomain(path, impl->m_buffer);

	auto & subdomain = parts.b;

	if (IsValidKey(parts.a))
	{
		REFLEX_RFOREACH(i, impl->m_list)
		{
			if (i.domain_id == parts.a)
			{
				auto range = i.m_minmax_subdomains;

				if (subdomain.size >= range.a && subdomain.size <= range.b)
				{
					if (auto instream = i.OnRead(subdomain, parts.c, attributes))
					{
						return instream;
					}
				}
			}
		}
	}
	else
	{
		REFLEX_RFOREACH(i, impl->m_list)
		{
			auto range = i.m_minmax_subdomains;

			if (subdomain.size >= range.a && subdomain.size <= range.b)
			{
				if (auto instream = i.OnRead(subdomain, path, attributes))
				{
					return instream;
				}
			}
		}
	}

	return {};
}

Reflex::UIntNative Reflex::File::VirtualFileSystem::Lock::PerformWrite(bool write, const WString::View & path, bool write_append, UIntNative fallback, WriteFn fn)
{
	REFLEX_ASSERT(path);

	auto impl = Cast<FileSystemImpl>(filesystem);

	impl->m_buffer.Clear();

	auto parts = Reflex::File::Detail::SplitDomain(path, impl->m_buffer);

	if (IsValidKey(parts.a))
	{
		REFLEX_RFOREACH(i, impl->m_list)
		{
			if (i.domain_id == parts.a)
			{
				if (auto result = fn(i, parts.b, parts.c, write_append))
				{
					return result;
				}
			}
		}
	}
	else
	{
		REFLEX_RFOREACH(i, impl->m_list)
		{
			if (i.domain_id == impl->m_defaultdomain)
			{
				if (auto result = fn(i, {}, path, write_append))
				{
					return result;
				}
			}
		}
	}

	return fallback;
}

Reflex::File::VirtualFileSystem::Locator::Locator(Key32 domain_id, Pair <UInt> minmax_subdomains)
	: domain_id(domain_id),
	m_minmax_subdomains(minmax_subdomains)
{
}

Reflex::File::VirtualFileSystem::Locator::~Locator()
{
	REFLEX_ASSERT(!GetList());
}

void Reflex::File::VirtualFileSystem::Lock::Attach(Locator & locator)
{
	auto impl = Cast<FileSystemImpl>(filesystem);

	Reference <Locator> retainer(locator);

	if (locator.GetList())
	{
		Lock current(locator.GetFileSystem());

		locator.Detach();
	}

	locator.Attach(impl->m_list);

	locator.m_filesystem = impl;
}

void Reflex::File::VirtualFileSystem::Lock::Detach(Locator & locator)
{
	if (locator.m_filesystem == filesystem)
	{
		locator.m_filesystem = VirtualFileSystem::null;

		locator.Detach();
	}
}

REFLEX_NOINLINE Reflex::Tuple <Reflex::Key32, Reflex::ArrayView <Reflex::WString::View>, Reflex::WString::View> Reflex::File::Detail::SplitDomain(const WString::View & path, Array <WString::View> & buffer)
{
	if (path && *path.data == L':')
	{
		if (auto idx = Search(path, kStroke))
		{
			auto domain_path = Splice(path, idx.value);

			auto domain = Nudge(domain_path.a);

			Reflex::Detail::Split(domain, L':', buffer);

			if (buffer)
			{
				auto subdomain = Splice(buffer, 1).b;

				Key32 domain_id = buffer.GetFirst();

				auto pathpart = Nudge(domain_path.b);

				return { domain_id, subdomain, pathpart };
			}
		}
	}

	return { {}, {}, path };
}
