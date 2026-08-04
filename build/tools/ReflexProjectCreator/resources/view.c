#module "System"
#module "File"
#module "ReflexProjectCreator"
#module "GLX > Ext"

#resource (stylesheet) "styles.glx" as gStyles;

#include "splash.h"
#include "interface.h"
#include "markup.h"

using GLX;

Interface iface = app#interface;

auto gGroupStyle = gStyles#Group;
auto gItemStyle = gStyles#Item;
auto gScrollerStyle = gStyles#Scroller;
auto gMarkupStyle = gScrollerStyle#Markup;

void Enter(Object obj)
{
	if (!obj#entered)
	{
		SetOpacity(obj, #enter_exit, 0.0f);
		
		obj#entered = true;
	}
	
	Run(obj, #enter_exit, 0.25f, CreateAnimation('Opacity', {#from: 0.0f, #to: 1.0f}));
}

void Exit(Object obj)
{
	Run(obj, #enter_exit, 0.25f, CreateAnimation('Opacity', {#from: 1.0f, #to: 0.0f}));
}

String GetLibraryPath(String id)
{
	auto path = iface.GetVariable(id);

	if (!path) path = File::ResolveIncludePath(iface.GetVariable('REFLEX-PATH'), "..");

	return GLX::ShowFolderDialog(false, path, Join("Locate ", id));
}

Tuple@(Object,Object) CreateWidget(Key32 type, String label)
{
	void FocusPrev(Object item)
	{
		if (auto prev = item.GetPrev())
		{
			prev.Focus();
		}
		else
		{
			item.GetParent().GetPrev().GetLastChild().Focus();
		}
	}

	void FocusNext(Object item)
	{
		if (auto next = item.GetNext())
		{
			next.Focus();
		}
		else
		{
			item.GetParent().GetNext().GetFirstChild().Focus();
		}
	}

	static const Key32 kActive = 'Active';

	auto item = Init(new, gItemStyle, label);

	item#KeyDown = [item](Object src, Event e)
	{
		switch (@UInt8 e#keycode)
		{
		case GLX::kKeyCodeTab:
			if ((@UInt8 e#modifiers) & 1)
			{
				FocusPrev(item);
			}
			else
			{
				FocusNext(item);
			}
			return true;

		case GLX::kKeyCodeUp:
			FocusPrev(item);
			return true;

		case GLX::kKeyCodeDown:
		case GLX::kKeyCodeEnter:
			FocusNext(item);
			return true;
		}

		return false;
	};
	
	auto child = Init(CreateWidget(type), gItemStyle[type]);

	item#Focus = [item, child]()
	{
		item.SetState(kActive);

		child.Focus();

		item#listener = CreateListener('Focus', [item](Object current)
		{
			if (BranchContains(item, current))
			{
				item.SetState(kActive);
			}
			else
			{
				item.ClearState(kActive);

				item.Clear@::Object('listener');
			}
		});
	};

	AddInlineFlex(item, child);

	return { item, child };
}

auto CreateProperty(Key32 type, Info info)
{
	auto item = CreateWidget(type, info.name);

	item.a.SetOnUpdate([auto id = info.id, auto child = item.b]()
	{
		auto value = iface.GetVariable(id);
		
		if (auto scroller = cast@GLX::Scroller(child))
		{
			scroller.GetContent().SetText(value);
		}
		else
		{
			child.SetText(value);
		}
	});

	item.a.Update();

	return item;
}

Object CreatePathProperty(Info info)
{
	auto item = CreateProperty('Button', info);

	auto button = item.b;

	button#MouseDown = [auto id = info.id]()
	{
		if (auto path = GetLibraryPath(id))
		{
			iface.SetVariable(id, path);
		}
	};

	return item.a;
}

Object CreateStringProperty(Info info)
{
	auto item = CreateProperty('TextEdit', info);

	auto obj = item.b;

	obj#Transaction = [auto id = info.id, obj](Event e)
	{
		if ((@UInt8 e#stage) == kTransactionEnd)
		{
			Scroller scroller = cast@GLX::Scroller(obj);

			Text text = scroller.GetContent()#value;

			iface.SetVariable(id, text.GetValue());
		}
	};

	return item.a;
}


auto body = new Object;

Init(body, gStyles);


auto header = Init(new, gGroupStyle);

header.SetFlow(kFlowY);

auto header_style = gStyles#Header;

auto bar = Init(new, header_style);

auto popup = CreateWidget('Popup');

popup#MenuOpen = [](Event e)
{
	Object menu = e#menu;

	AddMenuItem(menu, "Add Library")#MouseDown = []()
	{
		if (auto folder = GetLibraryPath("Library"))
		{
			iface.InstallLibrary(folder);
		}
	};
			
	AddMenuItem(menu, "Set Reflex Installation Path")#MouseDown = []()
	{
		iface.ClearTemplates();
	};

	if (ReflexProjectCreator::kDebug)
	{
		AddMenuItem(menu, "Reinstall (TEMP)")#MouseDown = []()
		{
			iface.Reinstall();
		};
	}
};

bar.AddFloat(Init(popup, header_style#Popup), kAlignmentRight);

header.AddInline(bar);

auto templates = CreateWidget('Popup', "Template");

header.AddInline(templates.a);


auto paths = Init(new, gGroupStyle);

paths.SetFlow(kFlowY);


auto strings = Init(new, gGroupStyle);

strings.SetFlow(kFlowY);


auto scroller = new Scroller;

Init(scroller, gStyles#Scroller);


auto build = Init(new, gStyles#Button);

build.SetMouseCursor(GLX::kMouseCursorPointer);


auto Recycle = [Map@(Key32,Object) recycle = new](Info info, Fn@(Object,Info) ctr)
{
	if (auto reuse = recycle.Query(info.id))
	{
		reuse.Update();

		return reuse;
	}
	else
	{
		auto obj = ctr(info);

		recycle[info.id] = obj;

		return obj;
	}

	return null;
};

self.SetOnUpdate([Recycle, auto templates = templates.b, paths, strings, scroller, build]()
{
	templates#MenuOpen = [](Event e)
	{
		Object menu = e#menu;

		if (auto templates = iface.GetTemplates())
		{
			foreach (info : iface.GetTemplates())
			{
				AddMenuItem(menu, info.name)#MouseDown = [info](Object src, Event e)
				{
					iface.SetTemplate(info.id);

					return false;
				};
			}
		}
	};

	body.Clear();

	if (auto t = iface.GetTemplates())
	{
		foreach (i : t) Print(i.name);
		
		body.SetFlow(kFlowY);

		body.AddInline(header);


		auto current = iface.GetTemplate();
			
		templates.SetText(current.info.name);
	
	
		paths.Clear();
	
		foreach (i : iface.GetPathVariables())
		{
			paths.AddInline(Recycle(i, bind CreatePathProperty));
		}
	
		body.AddInline(paths);
	
		if (paths.GetNumChild())
		{
			Enter(paths);
		}
		else
		{
			Exit(paths);
		}
	
		strings.Clear();
	
		foreach (i : iface.GetStringVariables())
		{
			strings.AddInline(Recycle(i, bind CreateStringProperty));
		}
	
		body.AddInline(strings);
	
	
		auto markup = ParseMarkup(Join(File::SplitFilename(current.path).a, "info.txt"), gMarkupStyle);
	
		scroller.SetContent(markup);
		
		body.AddInlineFlex(scroller);
	
	
		auto status = iface.CanBuild();
	
		if (status.a)
		{
			build.SetText("Create");
	
			build.SetState(kSelectedState);
	
			build#MouseDown = []()
			{
				auto current = iface.RestorePreference('view', 'dest', Join(System::kPathUserDocuments, "Reflex Projects", System::kPathDelimiter));
				
				if (auto folder = ShowFolderDialog(true, current, "Select Destination"))
				{
					iface.StorePreference('view', 'dest', folder);
					
					iface.Build(folder);
	
					RunReflexAnimation(self);
				}
			};
	
			build.EnableMouse(true, false);
		}
		else
		{
			build.SetText(status.b);
	
			build.ClearState(kSelectedState);
	
			build#MouseDown = null Fn@void;
	
			build.EnableMouse(false, false);
		}
	
		
		body.AddInlineFlex(scroller);

		body.AddInline(build);
	}
	else
	{
		auto text = Init(new, gStyles#Locate, "Please select the Reflex folder");
		
		body.SetFlow(kFlowY | kFlowCenter);
		
		body.AddInline(text);

		auto locate = Init(new, gStyles#Button);

		locate.SetMouseCursor(GLX::kMouseCursorPointer);
		
		locate.SetState(GLX::kSelectedState);
		
		locate.SetText("Locate");
		
		body.AddInline(locate, kOrientationCenter);
		
		locate#MouseDown = []()
		{
			if (auto path = ShowFolderDialog(false, ReflexProjectCreator::FindReflexPath(), "Locate Reflex"))
			{
				iface.InstallLibrary(path);
			}
		};
	}
});

body#KeyDown = [auto t = templates.a](Object src, Event e)
{
	switch (@UInt8 e#keycode)
	{
	case GLX::kKeyCodeTab:
		t.Focus();
		return true;

	case GLX::kKeyCodeUp:
	case GLX::kKeyCodeDown:
		t.Focus();
		return true;
	}

	return false;
};


self#resize = true;

self.AddStretch(body);

RunReflexAnimation(self);
