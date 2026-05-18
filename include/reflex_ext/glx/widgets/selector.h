#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class Selector;

}




//
//Selector

class Reflex::GLX::Selector : public Object
{
public:

	REFLEX_OBJECT(GLX::Selector, Object);


	
	//events 

	REFLEX_GLX_EVENT_ID(SelectPanel);


	
	//lifetime

	Selector();

	~Selector();



	//setup

	void EnableContentAutoFit(bool enable = true);	//compute false -> use style only



	//content

	void Clear();

	void AddPanel(TRef <Object> item, Key32 style_id = kcontent);

	void RemovePanel(UInt idx);


	UInt GetNumPanel() const;

	TRef <Object> GetPanel(UInt idx) const;



	//index

	void SelectPanel(UInt index);

	Idx GetCurrentIndex() const;

	

private:
	
	struct LayoutModel;

	using GLX::Object::GetNumItem;

	virtual void OnSetStyle(const Style & style) override;

	virtual void OnUpdate() override;	//forward to each panel

	template <bool COMPUTE> static void OnAccommodate(Selector & self, bool & isresponsive, Size & contentsize);


	FunctionPointer <void(Object&,Object&,bool,bool)> m_set_content;

	bool m_autofit;

	Idx m_index;

	Array < Pair <Reference <Object>, Key32> > m_content;

};

REFLEX_SET_TRAIT(Reflex::GLX::Selector, IsSingleThreadExclusive);




//
//impl

inline Reflex::UInt Reflex::GLX::Selector::GetNumPanel() const
{
	return m_content.GetSize();
}

inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Selector::GetPanel(UInt idx) const
{
	if (idx < m_content.GetSize())
	{
		return m_content[idx].a;
	}

	return {};
}

inline Reflex::Idx Reflex::GLX::Selector::GetCurrentIndex() const
{
	return m_index;
}
