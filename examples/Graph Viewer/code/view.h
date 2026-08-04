#pragma once

#include "app.h"




//
//declarations

namespace GraphViewer
{

	class View;

}




//
//GraphViewer::View

class GraphViewer::View : public Bootstrap::View
{
public:

	static TRef <View> Create(App & app);



protected:

	using Bootstrap::View::View;

};
