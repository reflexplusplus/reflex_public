#module "Data > Format"
#module "Data > String"
#module "File"

#include "data.h"




//
//interface (return {project path, error})

Tuple@(String,String) InstallTemplate(Template tmpl, Variables paths, Variables strings, String dest);

Tuple@(String,String) InstallTemplateWrapper(Data::PropertySet dynamic)
{
	//wrapper for Thread which currently only supports one parameter

	return InstallTemplate(dynamic#tmpl, dynamic#paths, dynamic#strings, dynamic#dest);
}




//
//impl

void RunProcess(String cmd, Array@String args)
{
	auto process = new System::Process { cmd, args };
	
	while (!process.Completed())
	{
		System::Sleep(250);
	}
}

void Setup(String package)
{
	switch (System::kOperatingSystem)
	{
		case ('windows') 
		{
			auto script = Join(package, "setup.bat");
			if (System::Exists(script))
			{
				RunProcess(script, []);
			}
		}
		
		case ('macos') 
		{
			auto script = Join(package, "setup.command");
			if (System::Exists(script))
			{
				RunProcess("/bin/bash", [Join(kDoubleQuote, script, kDoubleQuote)]);
			}
		}
	}
}

typedef Map@(Key32,String) KeyMap;

typedef Tuple@(UTF8,UTF8) Substitution;

Array@String gErrors;

void SetError(String error)
{
	Print(error);

	gErrors.Push(error);
}

template () bool Search(auto array, auto value)
{
	foreach (i : array)
	{
		if (i == value)
		{
			return true;
		}
	}

	return false;
}

String SearchVariable(Variables vars, String id)
{
	Int32 idx;

	foreach (i : vars)
	{
		if (i.a == id)
		{
			return i.b;
		}

		idx++;
	}

	return null;
}

Array@Substitution BuildSubstitionList(Array@String ids, Variables variables)
{
	object ItemRef
	{
		String obj = null;
	};

	bool GetLongest(Array@String ids, ItemRef ref)
	{
		Int32 length = 0;

		Int32 idx = ids.size;

		while (idx--)
		{
			auto i = ids[idx];

			auto t = i.size;

			if (t > length)
			{
				ref.obj = i;

				length = t;

				ids.Remove(idx, 1);

				return true;
			}
		}

		return false;
	}

	Array@Substitution rtn;

	auto clone = ids.Copy();

	ItemRef ref;

	while (GetLongest(clone, ref))
	{
		auto id = ref.obj;

		if (String to = SearchVariable(variables, id))
		{
			rtn.Push({Data::EncodeUTF8(Join("_", id, "_")), Data::EncodeUTF8(to)});
		}
		else
		{
			SetError(Join("missing value for ", id));
		}
	}

	return rtn;
}

void ValidateVariables(String desc, Variables variables)
{
	foreach (i : variables)
	{
		if (!i.b)
		{
			SetError(Join("Error generating ", i.a));
		}
	}
}

Variables ExpandPathVariables(Template tmpl, Variables paths)
{
	Variables expanded;

	foreach (i : tmpl.generators.paths)
	{
		auto input = File::RemoveTrailingStroke(SearchVariable(paths, i.id));

		switch (i.op)
		{
			case ('Relative')
			{
				expanded.Push({i.id, File::RemoveTrailingStroke(File::ResolveIncludePath(File::SplitFilename(tmpl.path).a, i.param))});
			}

			default:
			{
				SetError("Unknown generator op");
			}
		}
	}

	foreach (i : paths)
	{
		expanded.Push({i.a, File::RemoveTrailingStroke(i.b)});
	}

	ValidateVariables("Expanded Paths", expanded);

	return expanded;
}

Array@bool gValid = []()
{
	auto valid = new Array@bool;

	valid.SetSize(256);

	valid.Wipe();

	auto upper = 65;

	foreach (idx : 26) valid[upper++] = true;


	auto lower = 97;

	foreach (idx : 26) valid[lower++] = true;


	auto num = 48;

	foreach (idx : 10) valid[num++] = true;

	valid[95] = true;	//underscore

	return valid;
}();

void StripImpl(UTF8 utf8)
{
	auto n = utf8.size;

	while (n--)
	{
		if (!gValid[ToInt32(utf8[n])]) utf8.Remove(n,1);
	}
}

Variables ExpandStringVariables(Template tmpl, Variables strings)
{
	String StripImpl(String string)
	{
		auto utf8 = Data::EncodeUTF8(string);
	
		StripImpl(utf8);
	
		return Data::DecodeUTF8(utf8);
	}
	
	auto Strip = [strings](String var)
	{
		return StripImpl(SearchVariable(strings, var));
	};

	auto StripLowercase = [strings](String var)
	{
		return Lowercase(StripImpl(SearchVariable(strings, var)));
	};

	auto Generate4CC = [strings](String var)
	{
		auto string = SearchVariable(strings, var);

		auto hash = reinterpret@Tuple@(UInt8,UInt8,UInt8,UInt8)(@Key32 string);

		return Data::BytesToHex([hash.a ^ hash.b, hash.c ^ hash.d]);
	};


	auto expanded = strings.Copy();

	foreach (i : tmpl.generators.strings)
	{
		auto input = SearchVariable(strings, i.id);

		switch (i.op)
		{
			case ('Strip')
			{
				expanded.Push({ i.id, Strip(i.param) });
			}

			case ('StripLowercase')
			{
				expanded.Push({ i.id, StripLowercase(i.param) });
			}

			case ('Generate4CC')
			{
				expanded.Push({ i.id, Generate4CC(i.param) });
			}

			case ('Constant')
			{
				expanded.Push({ i.id, i.param });
			}

			default:
			{
				SetError("Unknown generator op");
			}
		}
	}

	ValidateVariables("Expanded Strings", expanded);

	return expanded;
}

void InstallFolder(Template tmpl, Variables variables, String src_path, String dest_path, String dest)
{
	bool EndsWith(String path, String end)
	{
		return Right(path, end.size) == end;
	}

	String RenameFile(Array@String rename_types, Array@Substitution renames, String path)
	{
		auto utf8 = Data::EncodeUTF8(path);

		foreach (i : rename_types)
		{
			if (EndsWith(path, i))
			{
				foreach (i : renames)
				{
					ReplaceAll(utf8, i.a, i.b);
				}
			}
		}

		return Data::DecodeUTF8(utf8);
	}

	String RenameFolder(Array@String rename_types, Array@Substitution renames, String path)
	{
		return File::CorrectTrailingStroke(RenameFile(rename_types, renames, File::RemoveTrailingStroke(path)));
	}

	auto targets = tmpl.targets;


	Array@Substitution content_substitutions = BuildSubstitionList(targets.replace_vars, variables);

	Array@Substitution rename_substitutions = BuildSubstitionList(targets.rename_vars, variables);


	auto dst = Join(dest_path, dest, System::kPathDelimiter);

	System::MakeDirectory(dst, false);

	if (System::IsDirectory(dst))
	{
		auto content = File::List(src_path, true);

		foreach (folder : content.a)
		{
			if (!Search(tmpl.targets.exclude_folders, folder))
			{
				auto folderpath = Join(src_path, folder);

				auto folder_renamed = RenameFolder(targets.rename_types, rename_substitutions, folder);

				InstallFolder(tmpl, variables, folderpath, dst, folder_renamed);
			}
		}

		foreach (filename : content.b)
		{
			if (!Search(tmpl.targets.exclude_files, filename))
			{
				auto filepath = Join(src_path, filename);

				auto filename_renamed = RenameFile(targets.rename_types, rename_substitutions, filename);

				auto dstpath = Join(dst, filename_renamed);

				if (File::CheckExtension(filename_renamed, targets.replace_types))
				{
					UTF8 bytes = File::Open(filepath);

					foreach (i : content_substitutions)
					{
						ReplaceAll(bytes, i.a, i.b);
					}

					File::Save(dstpath, bytes);
				}
				else
				{
					File::Copy(filepath, dstpath);
				}
			}
		}
	}
	else
	{
		SetError(Join("could not create ", dst));
	}
}

Tuple@(String,String) InstallTemplate(Template tmpl, Variables strings, Variables paths, String dest)
{
	bool HasTrailingStroke(String path)
	{
		auto pair = File::SplitFilename(path);

		if (pair.b)
		{
			return false;
		}

		return true;
	}

	String projectpath;

	if (HasTrailingStroke(dest))
	{
		auto expanded = Join(ExpandPathVariables(tmpl, paths), ExpandStringVariables(tmpl, strings));

		if (auto name = SearchVariable(expanded, kProductName))
		{
			//to git repository name
			auto name_utf8 = Data::EncodeUTF8(name);
			
			ReplaceAll(name_utf8, Data::EncodeUTF8(" "), Data::EncodeUTF8("_"));
			
			StripImpl(name_utf8);
			
			name = Lowercase(Data::DecodeUTF8(name_utf8));
			
			auto src = File::SplitFilename(tmpl.path).a;

			if (System::IsDirectory(src))
			{
				System::MakeDirectory(dest, true);

				if (System::IsDirectory(dest))
				{
					projectpath = Join(dest, name);

					InstallFolder(tmpl, expanded, src, dest, name);

					Setup(Join(projectpath, System::kPathDelimiter));
				}
				else
				{
					SetError("Could not create dest_folder");
				}
			}
			else
			{
				SetError("invalid template folder");
			}
		}
		else
		{
			SetError(Join(kProductName, " undefined"));
		}
	}
	else
	{
		SetError("invalid dest");
	}

	return { projectpath, gErrors[0] };
}