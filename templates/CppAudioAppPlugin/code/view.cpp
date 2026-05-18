#include "view.h"




//
//_PRODUCT-NAME-SYMBOL_::View implementation

namespace _PRODUCT-NAME-SYMBOL_ { namespace {	//begin internal namespace

using namespace Reflex;

class ViewImpl : public View
{
public:

	static constexpr UInt16 kChunkVersion = 0;	//change this to 1 to activate persistence callbacks

	ViewImpl(Instance & instance);


	//Bootstrap::View callbacks

	void OnResetState(Key32 context) override;

	void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	void OnStoreState(Data::Archive & stream) const override;


	//GLX::Object callbacks

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnSetStyle(const GLX::Style & style) override;

	void OnUpdate() override;


	
	const TRef <Instance> instance;


	GLX::Object m_header;

	GLX::Popup m_popup;

	GLX::Object m_body;

	GLX::Object m_controls;

	GLX::Button m_ide;
};

ViewImpl::ViewImpl(Instance & instance)
	: View(instance, kChunkVersion, L":res:_PRODUCT-NAME-SYMBOL_/styles.glx")
	, instance(instance)
{
	Data::SetBool(*this, GLX::kresize, false);	//set to true for resizable window

	GLX::AddInline(m_header, m_popup, GLX::kOrientationCenter);

	GLX::SetText(m_header, L"_PRODUCT-NAME_");

	GLX::AddInline(*this, m_header);

	auto num_param = instance.GetNumParameter();

	for (UInt idx = 0; idx < num_param; ++idx)
	{
		GLX::AddInline(m_controls, New<Bootstrap::ParamControl>(instance, idx));
	}

	GLX::AddFloat(m_body, m_controls, GLX::kAlignmentCenter);

	GLX::AddInlineFlex(*this, m_body);

	if constexpr (REFLEX_DEBUG)
	{
		GLX::SetText(m_ide, L"Console");

		GLX::AddFloat(m_header, m_ide, GLX::kAlignmentRight);
	}
}

void ViewImpl::OnResetState(Key32 context)
{
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
}

bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	 if (auto menu = GLX::GetMenu(e); menu && src == m_popup)
	{
		GLX::BindClick(menu->AddItem(L"New"), [this]()
		{
			instance->session->Reset();
		});

		GLX::BindClick(menu->AddItem(L"Open"), [this]()
		{
			if (auto path = GLX::ShowFileDialog(false, { Instance::kFileExt }, instance->GetFilename()))
			{
				instance->Open(path);
			}
		});

		GLX::BindClick(menu->AddItem(L"Save"), [this]()
		{
			if (auto path = GLX::ShowFileDialog(true, { Instance::kFileExt }, instance->GetFilename()))
			{
				instance->Save(path);
			}
		});

		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if constexpr (REFLEX_DEBUG)
		{
			if (src == m_ide)
			{
				Bootstrap::global->EnableIde(!Bootstrap::global->IdeEnabled());

				return true;
			}
		}
	}
	
	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto header_style = style["Header"];

	auto control_style = style["Control"];

	m_body.SetStyle(style["Body"]);

	m_header.SetStyle(header_style);

	m_popup.SetStyle(header_style["Popup"]);

	for (auto & i : m_controls) i.SetStyle(control_style);

	m_ide.SetStyle(style["Button"]);
}

void ViewImpl::OnUpdate()
{
	//this is called automatically when instance state changes

	WString title;

	if (auto filename = instance->GetFilename())
	{
		title = File::SplitExtension(File::SplitFilename(filename).b).a;
	}
	else
	{
		title = L"New";
	}

	if (instance->IsEdited()) title.Append(L" *");

	GLX::SetText(m_header, title);

	if constexpr (REFLEX_DEBUG) GLX::Select(m_ide, Bootstrap::global->IdeEnabled());
}

} }

Reflex::TRef <_PRODUCT-NAME-SYMBOL_::View> _PRODUCT-NAME-SYMBOL_::View::Create(Instance & instance)
{
	return New<ViewImpl>(instance);
}
