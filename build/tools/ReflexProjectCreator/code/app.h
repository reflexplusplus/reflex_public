#pragma once

#include "reflex_ext.h"
#include "../../ReflexCLI/code/common.h"




//
//declarations

namespace ReflexProjectCreator
{

	using namespace Reflex;


	using TokenDefinition = ReflexCLI::TokenDefinition;

	using TemplateDefinition = ReflexCLI::TemplateDefinition;

	
	class App;


	extern Output output;

}




//
//ReflexProjectCreator App

class ReflexProjectCreator::App : public Bootstrap::App
{
public:

	REFLEX_OBJECT(App, Bootstrap::App);

	static TRef <App> Create();


	virtual WString::View GetReflexPath() const = 0;

	//virtual void SetReflexPath(const WString & path) = 0;

	virtual void RefreshTemplates() = 0;

	virtual ArrayView <TemplateDefinition> GetTemplates() const = 0;

	virtual void InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, const WString & dest) = 0;



protected:

	using Bootstrap::App::App;

};
