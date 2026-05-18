#resource (stylesheet) "styles.txt" as styles;

#include "interface.h"
#include "helpers.h"

Interface iface = app#iface;

using GLX;

namespace Styles
{
	auto header = styles#Header;

	auto header_title = header#Title;

	auto button = styles#Button;
}

void OpenOrSave(bool save)
{
	if (auto path = GLX::ShowFileDialog(save, ["count"], app.GetFilename(), Select(save, "Save Preset", "Open Preset")))
	{
		if (save) 
		{
			app.Save(path);
		}
		else 
		{	
			app.Open(path);
		}
	}
}
	
Object CreateButton(String name, Fn@(void) onclick)
{
	auto button = Init(new, Styles::button, name);

	button.SetMouseCursor(kMouseCursorPointer);

	button#MouseDown = onclick;

	return button;
}


self.SetStyle(styles);

self.SetFlow(kFlowY);

self#resize = true;


auto buttons = new Object;

buttons.SetFlow(kFlowY);

auto count = Init(new, Styles::button);

buttons.AddInline(CreateButton("Inc", []()
{
	iface.IncCount(1);
}));

buttons.AddInline(CreateButton("Dec", []()
{
	iface.IncCount(-1);
}));

count.EnableMouse(false, false);

buttons.AddInline(count);

auto header = Init(new, Styles::header);

auto title = Init(new, Styles::header_title);

header.AddInlineFlex(title);

header.AddInline(CreateButton("New", []()
{
	app.Reset();
}));

header.AddInline(CreateButton("Open", bind OpenOrSave [ false ]));

header.AddInline(CreateButton("Save", bind OpenOrSave [ true ]));


self.AddInline(header);

self.AddFloat(buttons, kAlignmentCenter);



//start refresh mechanism

self.SetOnUpdate([title]()
{
	String text = "New";
	
	if (auto filename = app.GetFilename())
	{
		text = File::SplitFilename(filename).b;
	}

	if (app.IsEdited())
	{
		text = Join(text, " *");
	}

	title.SetText(text);
	
	count.SetText(ToString(iface.GetCount()));
});
