#module "Program"

template () void RunThread(Program program, Key32 name, auto p1, auto /*Fn@(void,auto)*/ callback)
{
	typedef typeof (callback) [1] RTN;
	typedef typeof (p1) P1;

	static Int32 counter;

	static Map@(Int32,Object) callbacks;

	Thread thread = { program, '', name, typeid RTN, [typeid P1], [@Object p1] };

	auto id = counter++;

	callbacks[id] = CreatePeriodicClock(0.25f, [thread, callback, id]()
	{
		if (auto result = thread.rtn)
		{
			callbacks.Remove(id);

			callback(cast@RTN(result));
		}
	});
}
