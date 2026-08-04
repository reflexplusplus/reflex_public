#pragma once

#include "../[require].h"




//
//Detail

namespace Reflex::IDE::Detail
{

	class ConsolePanel;


	class BuilderPanel;

	class ViewInspectorPanel;

	class DebugPanel;
	
}




//
//ConsolePanel

class Reflex::IDE::Detail::ConsolePanel : 
	public GLX::Object,
	public Data::iStreamable
{
public:

	REFLEX_OBJECT(ConsolePanel, GLX::Object);

	struct Ctr : public Reflex::Detail::StaticItem <Ctr>
	{
		Ctr(const WString::View & name, Int order, FunctionPointer <TRef<ConsolePanel>()> ctr) : name(name), order(order), ctr(ctr) {}

		WString::View name;

		Int order;

		FunctionPointer <TRef<ConsolePanel>()> ctr;
	};

	void Reset();

	void Deserialize(Data::Archive::View & stream);

	using Data::iStreamable::Serialize;



protected:

	ConsolePanel(Key32 id, UInt16 version);

	void SetIcon(const GLX::Style & style);


	void OnReset(Key32 context) override {}

	void OnRestore(Data::Archive::View & stream, Key32 context) override {}

	void OnStore(Data::Archive & stream) const override {}
	
};




//
//BuilderPanel

class Reflex::IDE::Detail::BuilderPanel : public ConsolePanel
{
public:

	REFLEX_OBJECT(BuilderPanel, ConsolePanel);

	virtual void RegisterWriteableDomain(Key32 domain) = 0;

	virtual bool SelectFile(Address address) = 0;



protected:

	using ConsolePanel::ConsolePanel;
};




//
//ViewInspectorPanel

class Reflex::IDE::Detail::ViewInspectorPanel : public ConsolePanel
{
public:

	REFLEX_OBJECT(ViewInspectorPanel, ConsolePanel);

	virtual void SetRoot(GLX::Object & object) = 0;



protected:

	using ConsolePanel::ConsolePanel;
};




//
//DebugPanel

class Reflex::IDE::Detail::DebugPanel : public ConsolePanel
{
public:

	REFLEX_OBJECT(DebugPanel, ConsolePanel);

	virtual void Config(UInt8 flags) = 0;



protected:

	using ConsolePanel::ConsolePanel;
};
