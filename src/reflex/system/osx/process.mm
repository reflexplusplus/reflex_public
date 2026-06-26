#include "sdk.h"
#include "../common/apple_utils.hpp"
#include "../common/posix/file.h"



//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

struct ProcessImpl : public System::Process
{
	ProcessImpl(const WString & filename, const ArrayView <WString> & args, const Options & options);

	~ProcessImpl();

	bool Status() const override { return m_status; }

	bool Completed() const override;

	void Wait() override;

	void Terminate() override;


	ObjCRef <NSTask*> m_nstask;

	bool m_status;

	ObjCRef <NSFileHandle*> m_std_out_handle;

	Reference <System::FileHandle> m_std_out;
};

ProcessImpl::ProcessImpl(const WString & path, const ArrayView <WString> & args, const Options & options)
{
	constexpr WChar kDoubleQuote = L'"';

	auto utf8 = ResolveBundlePath(path);

	auto nspath = MakeOwnedObjCRef([[NSString alloc] initWithBytes:utf8.GetData() length:utf8.GetSize() encoding:NSUTF8StringEncoding]);

	auto nsargs = MakeObjCRef([NSMutableArray array]);

	for (auto & arg : args)
	{
		if (auto i = ToView(arg))
		{
			if (i.GetFirst() == kDoubleQuote && i.GetLast() == kDoubleQuote)
			{
				i.data++;
				i.size -= 2;
			}

			[nsargs addObject:ToNSString(i)];
		}
	}

	m_nstask = MakeOwnedObjCRef([NSTask new]);

	[m_nstask setLaunchPath:nspath];

	[m_nstask setArguments:nsargs];

	if (auto std_out = options.std_out)
	{
		if (auto fd = Common::POSIX::QueryWriteableFileDescriptor(*std_out); fd != -1)
		{
			if (auto process_fd = dup(fd); process_fd != -1)
			{
				m_std_out_handle = MakeOwnedObjCRef([[NSFileHandle alloc] initWithFileDescriptor:process_fd closeOnDealloc:YES]);

				[m_nstask setStandardOutput:m_std_out_handle.Get()];

				m_std_out = std_out;
			}
		}
	}

	if constexpr (!REFLEX_DEBUG)
	{
		NSFileHandle * nullFileHandle = [NSFileHandle fileHandleWithNullDevice];

		if (!options.std_out)
		{
			[m_nstask setStandardOutput:nullFileHandle];
		}

		[m_nstask setStandardError:nullFileHandle];
	}

	m_status = true;

	@try
	{
		[m_nstask launch];
	}
	@catch (NSException* exception)
	{
		DEV_ERROR("Process run failed", ToCStringView(exception.name), "Reason", ToCStringView(exception.reason));
		m_status = false;
	}
}

ProcessImpl::~ProcessImpl()
{
	[m_nstask waitUntilExit];

	//required special workaround to ensure file is closed now
	if (m_std_out_handle)
	{
		[m_std_out_handle synchronizeFile];
		[m_std_out_handle closeFile];
	}
}

bool ProcessImpl::Completed() const
{
	return !([m_nstask isRunning]);
}

void ProcessImpl::Wait()
{
	[m_nstask waitUntilExit];
}

void ProcessImpl::Terminate()
{
	[m_nstask terminate];
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Process> Reflex::System::Process::Create(const WString & path, const ArrayView <WString> & args, const Options & options)
{
	return REFLEX_CREATE(OSX::ProcessImpl, path, args, options);
}
