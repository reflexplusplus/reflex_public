struct Result
{
	bool valid;
	Int32 value;
};

bool opCast@bool(Result result)
{
	return result.valid;
}

template () Result Search(auto array, auto value, Int32 pos)
{
	auto n = array.size - pos;
	
	foreach (idx : n)
	{
		if (array[pos] == value)
		{
			return { true, pos };
		}
		
		pos++;
	}
	
	return {};
}

template () auto Select(bool t, auto a, auto b)
{
	if (t)
	{
		return a;
	}
	
	return b;
}
