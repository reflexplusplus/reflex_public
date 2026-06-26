#include "reflex/glx/window.h"
#include "reflex/glx/event/functions.h"
#include "reflex/glx/functions/input.h"

#include "library.h"




//
//window::container

struct Reflex::GLX::WindowClient::Container : public GLX::Object
{
	Container();

	void OnUpdate() override
	{
		content->Update();
	}

	bool OnEvent(Object & src, GLX::Event & e) override;


	TRef <GLX::Object> content;

	GLX::Object foreground;

	UInt m_animationid;
};

Reflex::GLX::WindowClient::WindowClient()
	: m_container(REFLEX_CREATE(Container))
{
	Core::WindowClient::SetContent(m_container);
}

Reflex::GLX::WindowClient::WindowClient(bool profile)
	: Core::WindowClient(profile)
	, m_container(REFLEX_CREATE(Container))
{
	SetAbstractProperty(*this, K32("library"), TheLibrary::Get());

	Core::WindowClient::SetContent(m_container);
}

Reflex::GLX::WindowClient::~WindowClient()
{
	AnimationScope scope(false);

	m_container->Clear();

	Core::WindowClient::SetContent(GLX::Object::null);
}

void Reflex::GLX::WindowClient::SetContent(TRef <GLX::Object> content)
{
	content->ComputeLayout();

	SetBackgroundColour(content->GetComputedStyle()->GetBackgroundColour());

	m_container->content = content;

	m_container->Clear();

	AddStretch(m_container, content);

	AddStretch(m_container, m_container->foreground);

	BeginEventForwarding(m_container, kKeyDown, content);

	AutoFit();
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::WindowClient::GetContent()
{
	return m_container->content;
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::WindowClient::GetForeground()
{
	return m_container->foreground;
}

void Reflex::GLX::WindowClient::SetPosition(const Point & position)
{
	SetRect({ position, GetRect().size });
}

void Reflex::GLX::WindowClient::AutoFit()
{
	auto block = Data::Detail::AcquireProperty<ObjectOf<UInt>>(m_container, kNullKey);

	if (block->value++)
	{
		REFLEX_ASSERT(false);
	}
	else
	{
		Core::Context context;

		auto content = m_container->content;

		auto contentsize = Detail::ComputeContentSize(content);

		auto rect = GetRect();

		bool resizable = Data::GetBool(content, kresizable);

		Size min = { resizable ? rect.size.w : 0.0f, resizable ? rect.size.h : 0.0f };

		rect.size = Max(min, contentsize);

		SetRect(rect);
	}

	block->value--;
}

void Reflex::GLX::WindowClient::OnClose()
{
	EmitCloseRequest(m_container->content);
}

Reflex::System::ScreenOrientation Reflex::GLX::WindowClient::OnGetScreenOrientation()
{
	return System::kScreenOrientationDefault;
}

Reflex::System::iSize Reflex::GLX::WindowClient::OnGetContentSize()
{
	Core::Context context;

	auto content_size = Detail::ComputeContentSize(m_container->content);

	return { Truncate(content_size.w), Truncate(content_size.h) };
}

void Reflex::GLX::WindowClient::OnSetRect(System::WindowDisplay state, const System::iRect & rect, const System::iRect & interactable, Int32 dpifactor)
{
	Core::Context context;

	Core::WindowClient::OnSetRect(state, rect, interactable, dpifactor);

	Emit(m_container->content, kWindowResize);
}

Reflex::GLX::WindowClient::Container::Container()
{
	AddStretch(*this, foreground);

	EnableMouse(foreground, false);

	EnableAutoFit(foreground, false, false);
}

bool Reflex::GLX::WindowClient::Container::OnEvent(Object & src, GLX::Event & e)
{
	if (e.id == kRequestAutoFit)
	{
		GetWindow()->AutoFit();

		return true;
	}

	return Object::OnEvent(src, e);
}
