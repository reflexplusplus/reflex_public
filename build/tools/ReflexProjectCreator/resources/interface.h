#include "data.h"

object Interface
{
	Fn@(void,String) InstallLibrary;
	Fn@(void) Reinstall;

	Fn@(void,String) InstallTemplate;

	Fn@(void) ClearTemplates;
	Fn@(Array@Info) GetTemplates;

	Fn@(void,String) SetTemplate;
	Fn@(Template) GetTemplate;

	Fn@(Array@Info) GetPathVariables;
	Fn@(Array@Info) GetStringVariables;

	Fn@(void,Key32,String) SetVariable;
	Fn@(String,Key32) GetVariable;

	Fn@(void,Key32,Key32,String) StorePreference;
	Fn@(String,Key32,Key32,String) RestorePreference;

	Fn@(Tuple@(bool,String)) CanBuild;
	Fn@(Tuple@(bool,String),String) Build;
};