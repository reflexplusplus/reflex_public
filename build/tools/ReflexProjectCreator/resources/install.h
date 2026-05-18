//used by main.c app and also command line

#module "Data > Format"
#module "File"
#module "Program"
#module "ReflexProjectCreator"

#include "data.h"

#resource (program) "engine.c" as g_engine;




Info opCreate@Info(Data::PropertySet cfg)
{
	return { Data::GetCString(cfg, 'id'), Data::GetCString(cfg, 'name'), Data::GetCString(cfg, 'description') };
}

Generator opCreate@Generator(Data::PropertySet cfg)
{
	return { Data::GetCString(cfg, 'id'), cfg#op, Data::GetCString(cfg, 'param') };
}

Template OpenTemplate(String path)
{
	typedef Data::PropertySet PropertySet;

	Array@Generator ParseGenerators(Array@Data::PropertySet cfg)
	{
		auto self = new Array@Generator;

		foreach (i : cfg)
		{
			self.Push({i});
		}

		return self;
	}

	auto config = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(path));

	PropertySet info = config#info;

	if (String id = Data::GetCString(info, 'id'))
	{
		auto folder = File::SplitFilename(path).a;

		if (auto inherit = Data::GetCString(config, 'inherit'))
		{
			auto inheritpath = File::ResolveIncludePath(folder, inherit);
	
			auto parent = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(inheritpath));

			Data::Assimilate(parent, config);
			
			config = parent;
		}

		PropertySet input = config#input;
		PropertySet generate = config#generate;
		PropertySet exclude = config#exclude;
		PropertySet replace = config#replace;
		PropertySet rename = config#rename;

		auto rtn = new Template;

		rtn.info = { info };

		rtn.path = path;

		foreach (i : Data::GetPropertySetArray(input, 'paths'))
		{
			rtn.paths.Push({i});
		}

		foreach (i : Data::GetPropertySetArray(input, 'strings'))
		{
			rtn.strings.Push({i});
		}

		rtn.targets = 
		{ 
			Data::GetCStringArray(exclude, #folders), 
			Data::GetCStringArray(exclude, #files), 
			Data::GetCStringArray(replace, #types),
			Data::GetCStringArray(replace, #vars),
			Data::GetCStringArray(rename, #types),
			Data::GetCStringArray(rename, #vars)
		};

		rtn.generators = 
		{ 
			ParseGenerators(Data::GetPropertySetArray(generate, 'paths')), 
			ParseGenerators(Data::GetPropertySetArray(generate, 'strings')) 
		};

		return rtn;
	}

	return null;
}

Variables GetVariables(Data::PropertySet input, Array@Info array)
{
	Variables variables;

	foreach(i : array)
	{
		variables.Push({ i.id, Data::GetCString(input, i.id) });
	}

	return variables;
}

Data::PropertySet EncodeEngineArgs(Template tmpl, Data::PropertySet input, String dest)
{
	Variables paths = GetVariables(input, tmpl.paths);

	Variables strings = GetVariables(input, tmpl.strings);

	auto params =
	{
		'tmpl': tmpl,
		'paths' : paths,
		'strings' : strings,
		'dest' : dest
	};

	return params;
}
