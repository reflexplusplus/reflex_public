object Info
{
	String id, name, description;
};

object Generator
{
	String id;
	Key32 op;
	String param;
};

object Generators
{
	Array@Generator paths, strings;
};

object Targets
{
	Array@String exclude_folders, exclude_files;

	Array@String replace_types, replace_vars;

	Array@String rename_types, rename_vars;
};

typedef Array@Tuple@(String,String) Variables;

object Template
{
	String path;		//eg "d:/devt/library/reflex/templates/project/"

	Info info;

	Array@Info paths, strings;

	Generators generators;

	Targets targets;
};

const String kProductName = "PRODUCT-NAME";	//special hardcoded variable needed for building project path



//
//helpers

Tuple@(String,String) Splice(String string, Int32 position)
{
	return { Left(string, position), Right(string, string.size - position) };
}

Tuple@(String,String) ReverseSplice(String string, Int32 position)
{
	return { Left(string, string.size - position), Right(string, string.size) };
}

void Print(String key, String value)
{
	Print(Join(key,": ", value));
}

typedef Array@UInt8 UTF8;

void ReplaceAll(UTF8 bytes, UTF8 from, UTF8 to)
{
	Tuple@(UTF8,UTF8) Splice(UTF8 data, Int32 pos)
	{
		auto a = data.Share();

		auto b = data.Share();

		a.SetSize(pos);

		b.Nudge(pos);

		return {a, b};
	}

	auto clone = bytes.Share();

	auto pos = 0;

	while (SearchRegion(clone, from, pos))
	{
		auto t1 = Splice(clone, pos);

		auto t2 = Splice(t1.b, from.size);

		clone = Join(t1.a, to, t2.b);

		pos += to.size;
	}

	bytes.Clear();

	bytes.Append(clone);
}

