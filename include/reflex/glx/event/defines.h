#pragma once

#include "event.h"




//
//Primary API

#define REFLEX_GLX_EVENT_ID(ID) REFLEX_DECLARE_KEY32(ID)	//-> constexpr UInt32 kID = "ID"

namespace Reflex::GLX
{

	REFLEX_GLX_EVENT_ID(PointerDown);	//UInt8 slot, Float64 timestamp, UInt8 modifiers, UInt8 flags
	REFLEX_GLX_EVENT_ID(PointerDrag);	//UInt8 slot, Float64 timestamp, UInt8 modifiers, Point delta
	REFLEX_GLX_EVENT_ID(PointerUp);		//UInt8 slot, Float64 timestamp, UInt8 modifiers


	REFLEX_GLX_EVENT_ID(MouseEnter);
	REFLEX_GLX_EVENT_ID(MouseLeave);

	constexpr auto kMouseDown = kPointerDown;
	constexpr auto kMouseDrag = kPointerDrag;
	constexpr auto kMouseUp = kPointerUp;

	REFLEX_GLX_EVENT_ID(MouseWheel);	//Point delta


	REFLEX_GLX_EVENT_ID(Focus);
	REFLEX_GLX_EVENT_ID(LoseFocus);


	REFLEX_GLX_EVENT_ID(KeyDown);		//System::KeyCode keycode, bool repeat, UInt8 modifiers
	REFLEX_GLX_EVENT_ID(KeyUp);			//System::KeyCode keycode

	REFLEX_GLX_EVENT_ID(Character);		//WChar character


	REFLEX_GLX_EVENT_ID(DragDropTender);
	REFLEX_GLX_EVENT_ID(DragDropEnter);
	REFLEX_GLX_EVENT_ID(DragDropLeave);
	REFLEX_GLX_EVENT_ID(DragDropReceive);

	REFLEX_GLX_EVENT_ID(DragDropReceiveExternal);


	REFLEX_GLX_EVENT_ID(Transaction);


	REFLEX_GLX_EVENT_ID(RequestClose);		//emitted by WindowClient content when window close button is clicked, also used by dialogs and overlays to request "release" by owner



	//common parameters

	REFLEX_DECLARE_KEY32(flags);
	REFLEX_DECLARE_KEY32(pointer_slot);
	REFLEX_DECLARE_KEY32(timestamp);
	REFLEX_DECLARE_KEY32(modifiers);	//mouse and key messages
	REFLEX_DECLARE_KEY32(capture);
	REFLEX_DECLARE_KEY32(position);		//PointerDown, PointerDrag, PointerUp
	REFLEX_DECLARE_KEY32(delta);		//MouseDrag, MouseWheel
	REFLEX_DECLARE_KEY32(inverted);		//MouseWheel
	REFLEX_DECLARE_KEY32(keycode);		//KeyDown/KeyUp
	REFLEX_DECLARE_KEY32(drag_data);
	REFLEX_DECLARE_KEY32(menu);
	REFLEX_DECLARE_KEY32(context);
	REFLEX_DECLARE_KEY32(stage);
	REFLEX_DECLARE_KEY32(index);
	REFLEX_DECLARE_KEY32(item);
	REFLEX_DECLARE_KEY32(allow);

	REFLEX_DECLARE_KEY32(state);

}
