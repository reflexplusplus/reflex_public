#module "Data > String"

#include "interface.h"
#include "install.h"
#include "thread.h"




//
//globals

Data::PropertySet gPrefs, gSession;

Map@(Key32,Template) gTemplates;

Template gCurrent;




//
//Bootstrap persistence callbacks

void OnReset()
{
	gSession = new;
}

void OnRestore(Data::BinaryObject chunk)
{
	gSession = Data::DecodePropertySet(Data::kPropertySetFormat, chunk);

	if (auto cfg = gTemplates.Query(Data::GetCString(gSession, 'current')))
	{
		gCurrent = cfg;
	}
	else
	{
		gCurrent = {};
	}
}

Data::BinaryObject OnStore()
{
	return Data::EncodePropertySet(Data::kPropertySetFormat, gSession);
}




//
//

void InstallLibrary(String library_path);

void Commit()
{
	Data::SetBinary(prefs, 'state', Data::EncodePropertySet(Data::kPropertySetFormat, gPrefs));

	self.Notify(true);
}

Array@Info GetTemplates()
{
	Array@Info rtn;

	foreach (k,v : gTemplates)
	{
		rtn.Push(v.info);
	}

	return rtn;
}

bool InstallTemplate(String template_path)
{
	Print("InstallTemplate", template_path);

	if (auto tmpl = OpenTemplate(template_path))
	{
		String id = tmpl.info.id;

		gTemplates[id] = tmpl;

		Array@String installed;

		foreach (k,i : gTemplates)
		{
			installed.Push(i.path);
		}

		Data::SetCStringArray(gPrefs, 'installed', installed);

		Data::SetCString(gSession, 'current', tmpl.info.id);

		gCurrent = tmpl;

		Commit();

		return true;
	}

	return false;
}

void InstallLibrary(String library_path)
{
	Print("InstallLibrary", library_path);

	auto templates_path = Join(library_path, "templates", System::kPathDelimiter);

	Data::PropertySet library_info = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(Join(templates_path, "library.cfg")));

	if (library_info)
	{
		//store library path (saves user from having to set it manually for other libs eg netx)

		String id = Data::GetCString(library_info, 'id');

		auto path = Left(library_path, library_path.size - 1);	//remove trailing stroke

		Print(Join(Data::GetCString(library_info, 'id', null), "-PATH"), path);

		Data::SetCString(gSession, Join(Data::GetCString(library_info, 'id', null), "-PATH"), path);


		//no longer working after rebuild on tahoe, abandoning as too fragile

		// //post install steps

		// Data::PropertySet post_install_steps = library_info2['post_install_steps'];

		// auto platform_steps = Data::GetPropertySetArray(post_install_steps, ReflexProjectCreator::GetPlatformName());

		// if (platform_steps)
		// {
		// 	auto installPathUtf8 = Data::EncodeUTF8("{{INSTALL_PATH}}");
		// 	auto pathUtf8 = Data::EncodeUTF8(path);

		// 	Print("Running post install steps");

		// 	foreach (l : platform_steps)
		// 	{
		// 		String commandPath = l#path;

		// 		Array@String args = l#args;
			
		// 		foreach (index : args.size) 
		// 		{
		// 			auto str = Data::EncodeUTF8(args[index]);
		// 			ReplaceAll(str, installPathUtf8, pathUtf8);
		// 			args[index] = Data::DecodeUTF8(str);
		// 		}

		// 		auto task = new System::Process { commandPath, args };
		// 	}
		// }

		foreach (i : File::List(templates_path, false).a)
		{
			InstallTemplate(Join(templates_path, i, "install.cfg"));
		}
	}
}




//
//publish interface

Interface interface =
{
	.InstallLibrary = [](String folder)
	{
		InstallLibrary(folder);
	},

	.Reinstall = []()
	{
		InstallLibrary(Join(Data::GetCString(gSession, 'REFLEX-PATH'), System::kPathDelimiter));
	},

	.InstallTemplate = [](String folder)
	{
		InstallTemplate(Join(folder, "install.cfg"));
	},

	.ClearTemplates = []()
	{
		gTemplates = new;

		gCurrent = new;

		gSession = new;

		gPrefs = new;

		Commit();
	},

	.GetTemplates = bind GetTemplates,

	.SetTemplate = [] void (String id)
	{
		foreach (k,v : gTemplates)
		{
			if (v.info.id == id)
			{
				auto tmpl = OpenTemplate(v.path);

				Data::SetCString(gSession, 'current', tmpl.info.id);

				gCurrent = tmpl;

				Commit();

				if (!tmpl)
				{
					//todo remove
				}
			}
		}
	},

	.GetTemplate = [] Template ()
	{
		return gCurrent;
	},

	.GetPathVariables = [] Array@Info ()
	{
		return gCurrent.paths;
	},

	.GetStringVariables = [] Array@Info ()
	{
		return gCurrent.strings;
	},

	.SetVariable = [] void (Key32 id, String value)
	{
		foreach (x : [gCurrent.paths, gCurrent.strings])
		{
			foreach (i : x)
			{
				if (id == i.id)
				{
					Data::SetCString(gSession, id, value);

					Commit();
				}
			}
		}
	},

	.GetVariable = [](Key32 id)
	{
		return Data::GetCString(gSession, id);
	},

	.StorePreference = [](Key32 client, Key32 id, String value)
	{
		Data::SetCString(gPrefs[client], id, value);

		Commit();
	},

	.RestorePreference = [](Key32 client, Key32 id, String fallback)
	{
		return Data::GetCString(gPrefs[client], id, fallback);
	},

	.CanBuild = [] Tuple@(bool,String) ()
	{
		auto paths = GetVariables(gSession, gCurrent.paths);

		auto strings = GetVariables(gSession, gCurrent.strings);

		foreach (group : [paths, strings])
		{
			foreach (i : group)
			{
				if (!i.b)
				{
					return { false, Join(i.a, " undefined")};
				}
			}
		}

		foreach (i : paths)
		{
			if (!System::IsDirectory(i.b))
			{
				return { false, Join(i.a, " is not a valid folder")};
			}
		}

		Tuple@(bool,String) result = { true, null String };
		
		switch (@Key32 interface.GetVariable(kProductName))	//prevent app names that break template
		{
		case 'App':
		case 'Reflex':
		case 'System':
		case 'Data':
		case 'File':
		case 'Debug':
		case 'GLX':
		case 'IDE':
			result = { false, "Invalid Product Name" };
			break;
		};
		
		return result;
	},

	.Build = [] Tuple@(bool,String) (String dest)
	{
		auto params = EncodeEngineArgs(gCurrent, gSession, dest);

		Tuple@(bool, String) result;

		RunThread(g_engine, 'InstallTemplateWrapper', params, [dest, result](Tuple@(String, String) path_error)
		{
			result.a = path_error.a;
			result.b = path_error.b;

			if (path_error.a)
			{
				System::Open(Join(path_error.a, System::kPathDelimiter, "project", System::kPathDelimiter, ReflexProjectCreator::GetPlatformName()));
			}
		});

		return {};
	}
};

self#interface = interface;

gPrefs = Data::DecodePropertySet(Data::kPropertySetFormat, Data::GetBinary(prefs, 'state'));

foreach (i : Data::GetCStringArray(gPrefs, 'installed'))
{
	if (auto tmpl = OpenTemplate(i))
	{
		gTemplates[tmpl.info.id] = tmpl;
	}
}

