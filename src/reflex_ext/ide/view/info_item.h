#pragma once

#include "../globalimpl.h"




//
//

REFLEX_NS(Reflex::IDE)

struct InfoItem : public GLX::Object
{
	InfoItem(const WString & wkey, const WString::View & wvalue = {}, bool path = false)
		: ispath(path)
	{
		GLX::SetText(key, wkey);

		GLX::SetText(value, wvalue);

		GLX::EnableAutoFit(*this, false, true);

		GLX::EnableAutoFit(value, false, true);

		//SetProperty(1, Null<Object>());

		GLX::AddInline(*this, key);

		GLX::AddInlineFlex(*this, value);

		GLX::EnableMouse(key, false);

		GLX::EnableMouse(value, false);
	}

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kMouseDown)
		{
			if (GLX::GetClickFlags(e) & GLX::kClickFlagRmb)
			{
				auto menu = GLX::OpenContextMenu(*this);

				WString text = GLX::GetText(value);

				if (ispath)
				{
					GLX::BindClick(menu->AddItem(L"Open Location"), [text]()
					{
						System::Open(File::SplitFilename(text).a);
					});
				}

				GLX::BindClick(menu->AddItem(L"Copy"), [text]()
				{
					System::SetClipboard(text);
				});

				return true;
			}
		}

		return GLX::Object::OnEvent(src, e);
	}

	virtual void OnSetStyle(const GLX::Style & style) override
	{
		key.SetStyle(style[K32("Key")]);

		value.SetStyle(style[K32("Value")]);
	}

	const bool ispath;

	GLX::Label key, value;
};

REFLEX_END
