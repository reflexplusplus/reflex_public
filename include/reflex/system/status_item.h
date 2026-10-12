#pragma once

#include "types.h"




//
//Experimental API

namespace Reflex::System
{

	class StatusItem;	//a macOS menu bar item or a Windows notification area item. UI thread only.

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

	static BitmapInfo GetPreferredBitmapInfo();		//size and density of the status area; format is kImageFormatLuminance

	[[nodiscard]] static Unretained <StatusItem> Create();	//null on platforms without a status area



	//callbacks

	//the first successful call publishes the native item. luminance bitmaps are template (macOS) or theme tinted (Windows) icons.
	//returns false for invalid bitmap data or a native failure.

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

	virtual void OnClick(bool rmb, bool dbl) {}	//on button release; dbl is the second click of a double click
};
