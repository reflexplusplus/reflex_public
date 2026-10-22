#pragma once

#include "types.h"




//
//Experimental API

namespace Reflex::System
{

	class StatusItem;	//macOS menu bar | Windows notification area | Linux StatusNotifierItem

}




//
//StatusItem

class Reflex::System::StatusItem : public Object
{
public:

	REFLEX_OBJECT(System::StatusItem, Object);

	static StatusItem & null;



	//types

	struct MenuItem
	{
		WString label;				//empty label is a separator
		Function <void()> action;	//no action is a disabled item
	};

	class Client;



	//lifetime

	static BitmapInfo GetPreferredBitmapInfo();		//suggested size and density

	[[nodiscard]] static Unretained <StatusItem> Create();



	//callbacks

	virtual bool SetBitmap(const RawBitmap & bitmap) = 0;

	virtual void SetMenu(ArrayView <MenuItem> items) = 0;	//rebuilds the native menu; an empty array removes it

	virtual void SetTooltip(const WString & text) = 0;

	virtual void SetClient(WillRetain <Client> client) = 0;	//receives clicks when there is no menu
};




//
//StatusItem::Client

class Reflex::System::StatusItem::Client : public Object
{
public:

	static Client & null;

	virtual void OnClick(bool rmb, bool dbl) {}
};
