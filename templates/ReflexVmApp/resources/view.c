#resource (stylesheet) "styles.glx" as styles;

#include "interface.h"



//get interface

Interface iface = app#iface;



//build layout

using GLX;

self.SetStyle(styles);

self.SetFlow(kFlowY);

self#resize = true;



//update on changes

self.SetOnUpdate([]()
{
	//App state changed, update the view
});
