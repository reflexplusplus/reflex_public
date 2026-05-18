#module "Data > Serialize"
#module "File"

//example interface to expose to view

object Interface
{
	Fn@(void) ResetCount = null;
	Fn@(void,Int32) IncCount = null;
	Fn@(Int32) GetCount = null;
};

