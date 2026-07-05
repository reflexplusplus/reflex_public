#include "../../../../include/reflex_ext/bootstrap/common/global.h"

#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
#include "../../../../include/reflex_ext/ide.h"
#endif




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

constexpr UInt32 kIDE = MakeKey32("bootstrap.ide");

struct GlobalImpl : public Global
{
	GlobalImpl(CString::View vendor, CString::View product, WString::View project_dir, Key32 resources_subdomain);

	~GlobalImpl();

	TRef <Object> CreateDeepLinkListener(const Function<void(CString::View)> & callback) override;

	TRef <Object> EnableIde(bool enable) override;

	bool IdeEnabled() const override;

	void CommitPreferences() override;

#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
	void OnSetProperty(Address address, Object & object) override;


	REFLEX_IF_DEBUG(WString m_project_dir;)

	UInt8 m_ide_enabled;

	UInt8 m_flushcycle;

	State::Monitor m_prefs_monitor;

	Reference <Object> m_ide;

	Reference <Object> m_autoflushclock;

	SignalComponent <CString::View> m_deeplink_signal;
#endif
};

GlobalImpl::GlobalImpl(CString::View vendor, CString::View product, WString::View project_dir, Key32 resources_subdomain)
	: Global(vendor, product, project_dir, resources_subdomain)
#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
	REFLEX_IF_DEBUG(,m_project_dir(project_dir))
	, m_ide_enabled(kMaxUInt8)
	, m_flushcycle(0)
	, m_prefs_monitor(prefs)
	, m_autoflushclock(Async::CreateClock(this, [](GlobalImpl & self)
	{
		if (!(++self.m_flushcycle & 15))	//~4 x per sec
		{
			File::ResourcePool::Lock lock(self.resourcepool);

			if (GLX::module.IsInitalised())
			{
				GLX::Core::Context ctx;

				lock.PurgeIncremental();
			}
			else
			{
				lock.PurgeIncremental();
			}

			if constexpr (REFLEX_DEBUG)
			{
				if (self.m_prefs_monitor.Poll())
				{
					self.CommitPreferences();
				}
			}
		}
	}))
#endif
{
#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
	if constexpr (REFLEX_DEBUG)
	{
		if (!prefs->QueryProperty<Data::BoolProperty>(kIDE))
		{
			Data::SetBool(prefs, kIDE, true);
		}
	}

	EnableIde(Data::GetBool(prefs, kIDE));
#endif

	module.Init();				//initialise dependent modules
}

GlobalImpl::~GlobalImpl()
{
	module.Deinit();			//deinitialise dependent modules

	Data::PropertySet::Clear();	//destroy singletons and products before libs and freestyle global deestroyed

	CommitPreferences();

	Release(prefs);

	Release(resourcepool);
}

TRef <Object> GlobalImpl::EnableIde(bool enable)
{
#ifdef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
	return {};
#else
	if (SetFiltered(m_ide_enabled, UInt8(enable)))
	{
		if (enable)
		{
			WString logfile;

			if (Data::GetBool(prefs, K32("bootstrap.log_file")))
			{
				logfile = Detail::MakeProductPath(System::kPathDesktop, vendor, product);
				File::MakePath(logfile);
				logfile.Append(ToWString(System::GetTime()));
				logfile.Push(File::kDot);
				logfile.Append(IDE::kTXT);
			}
			else
			{
				#if REFLEX_DEBUG
				logfile = Join(m_project_dir, L"reflex_log.txt");
				Reflex::g_default_allocator->SetLeakLogPath(Join(m_project_dir, L"reflex_leaks.txt"));
				#endif
			}

			if (logfile)
			{
				Output::SetOutputFile(New<System::FileHandle>(logfile, System::FileHandle::kModeOverwrite));
			}

			m_ide = IDE::Start(resourcepool, prefs);
		}
		else
		{
			m_ide.Clear();

			Output::Disable();
		}

		Data::SetBool(prefs, kIDE, enable);

		GLX::Restart();
	}

	return m_ide;
#endif
}

bool GlobalImpl::IdeEnabled() const
{
#ifdef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
	return false;
#else
	return True(m_ide);
#endif
}

void GlobalImpl::CommitPreferences()
{
	prefs->Save(Detail::MakeProductPath(System::kPathUserData, vendor, product, Detail::kPrefsFilename));
}

#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
void GlobalImpl::OnSetProperty(Address address, Object & object)
{
	if (address == MakeAddress<Data::CStringProperty>("DeepLinkUrl"))
	{
		m_deeplink_signal.Notify(Cast<Data::CStringProperty>(object)->value);

		AutoRelease(object);
	}
	else
	{
		PropertySet::OnSetProperty(address, object);
	}
}
#endif

TRef <Object> GlobalImpl::CreateDeepLinkListener(const Function<void(CString::View)> & callback) 
{
#ifdef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
	return {};
#else
	return m_deeplink_signal.CreateListener(callback);
#endif
}

typedef Reflex::The <GlobalImpl> TheGlobal;

REFLEX_END_INTERNAL

Reflex::Detail::Module Reflex::Bootstrap::module("Reflex::Bootstrap");

Reflex::WString Reflex::Bootstrap::Detail::MakeProductPath(System::Path system_path, const CString::View & vendor, const CString::View & name, const WString::View & filename)
{
	return Join(System::GetPath(system_path), ToWString(vendor), File::kStroke, ToWString(name), File::kStroke, filename);
}

Reflex::TRef <Reflex::Bootstrap::Global> Reflex::Bootstrap::Global::Acquire(CString::View vendor, CString::View product, WString::View projectdir, Key32 resources_subdomain)
{
	REFLEX_ASSERT_MAINTHREAD("Bootstrap::Global::Acquire");

	return TheGlobal::Acquire(vendor, product, projectdir, resources_subdomain);
}

Reflex::Bootstrap::Global::Global(CString::View vendor, CString::View product, WString::View project_dir, Key32 resources_subdomain)
	: filesystem(File::VirtualFileSystem::Create(File::kdisk)),
	resourcepool(File::ResourcePool::Create(filesystem)),
	prefs(New<File::PersistentPropertySet>(Data::kPropertySetFormat)),
	vendor(vendor),
	product(product)
{
	Retain(resourcepool);

	Retain(prefs);

	REFLEX_ASSERT(vendor && product);

	{
		File::VirtualFileSystem::Lock lock(filesystem);

		lock.Attach(New<File::EnumerableEmbeddedResource::Locator>());

		lock.Attach(New<File::FileLocator>());

#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
		if (project_dir) lock.Attach(New<IDE::ProxyPath>(MakeKey32("res"), resources_subdomain, Join(project_dir, L"resources", System::kPathDelimiter)));
#endif
	}

	auto path = Detail::MakeProductPath(System::kPathUserData, vendor, product, Detail::kPrefsFilename);

	File::MakePath(File::SplitFilename(path).a);

	if (!System::Exists(path))	//TEMP migration
	{
		constexpr WString::View kOldFilenames[] = { L"config", L"state" };

		for (auto & i : kOldFilenames)
		{
			auto old_path = Detail::MakeProductPath(System::kPathUserData, vendor, product, i);

			if (System::Exists(old_path))
			{
				path = old_path;

				break;
			}
		}
	}

	prefs->Open(path);
}

const Reflex::TRef <Reflex::Bootstrap::Global> Reflex::Bootstrap::global = Reflex::Bootstrap::TheGlobal::Get<true>();
