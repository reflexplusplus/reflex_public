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

	virtual ArrayView <TemplateDefinition> GetTemplates() const = 0;

	virtual ArrayView <Pair<CString,bool>> GetTargets() const = 0;

	virtual void InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, ArrayView <CString> targets, const WString & dest, bool overwrite) = 0;



protected:

	using Bootstrap::App::App;

};
