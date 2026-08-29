#include "session.h"




//
//main

using namespace Reflex;

void _PRODUCT-NAME-SYMBOL_::Main(ConsoleSession & session, File::VirtualFileSystem::Lock & lock)
{
	if (Search(session.GetCommandLine(), "--test"))
	{
		session.Print("*** Test ***");
	}
	else
	{
		auto string = session.GetInput("Type 'world'");

		session.Print(Join("Hello ", string));

		session.GetInput("Press return to exit");
	}
}
