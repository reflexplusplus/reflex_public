#include "console.h"

#include "debug/log.cpp"
#include "debug/profiler.cpp"
#include "debug/resources.cpp"
#include "debug/memory.cpp"



//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct DebugPanelImpl : public Detail::DebugPanel
{
	template <class T>
	struct Creator
	{
		static TRef <Detail::ConsolePanel> Create() { return REFLEX_CREATE(T); }
	};

	DebugPanelImpl();

	void Config(UInt8 flags) override;

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		for (auto & i : m_panels) Data::Deserialize(stream, *i);

		auto pair = MakeTuple(UInt32(0), Idx(0));

		Data::Deserialize(stream, pair);

		if (pair.b.value < m_panels.GetSize()) m_tabgroup.GetSelector()->SelectPanel(pair.b.value);
	}

	void OnStore(Data::Archive & stream) const override
	{
		for (auto & i : m_panels) Data::Serialize(stream, *i);

		Data::Serialize(stream, MakeTuple(UInt32(0), m_tabgroup.GetSelector()->GetCurrentIndex().value));
	}

	void OnSetProperty(Address address, Reflex::Object & object) override
	{
		if (address == MakeAddress<Data::UInt8Property>(K32("Config")))
		{
			Config(Cast<Data::UInt8Property>(object)->value);
		}

		Detail::ConsolePanel::OnSetProperty(address, object);
	}

	Array <IDE::Detail::ConsolePanel*> m_panels;// , profile, resources, filesystem;

	GLX::TabGroup m_tabgroup;
};

DebugPanelImpl::DebugPanelImpl()
	: Detail::DebugPanel("Debug", 1)
{
	auto styles = Detail::RetrieveStyleSheet();

	SetProperty(kNullKey, styles.RemoveConst());	//to use in config

	m_tabgroup.SetStyle(styles["ConsoleGroup"]);

	Config(kMaxUInt8);

	for (auto & i : m_panels) i->SetStyle(styles);

	GLX::AddStretch(*this, m_tabgroup);
}

void DebugPanelImpl::Config(UInt8 flags)
{
	flags = flags & 7;

	if (flags)
	{
		FunctionPointer <TRef<IDE::Detail::ConsolePanel>()> ctrs[] =
		{
			&Creator<LogView>::Create,
			&Creator<ProfileView>::Create,
			&Creator<FilesView>::Create,
			&Creator<FileSystemView>::Create,
			&Creator<MemoryView>::Create,
		};

		WString::View labels[] = { L"Log", L"Profile", L"Resources", L"VirtualFileSystem", L"Memory" };

		auto & styles = *QueryProperty<GLX::StyleSheet>(kNullKey);

		m_panels.Clear();

		m_tabgroup.Clear();

		REFLEX_LOOP(idx, GetArraySize(ctrs))
		{
			//if (BitCheck(flags, idx))
			{
				auto panel = ctrs[idx]();

				panel->SetStyle(styles);

				m_panels.Push(panel.Adr());

				m_tabgroup.AddPanel(labels[idx], panel);
			}
		}

		m_tabgroup.GetSelector()->SelectPanel(0);
	}
}

Detail::ConsolePanel::Ctr gDebugCtr(L"Debug", -1, []() -> TRef <Detail::ConsolePanel>
{
	return REFLEX_CREATE(DebugPanelImpl);
});

REFLEX_END_INTERNAL
