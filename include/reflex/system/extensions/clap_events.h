#pragma once

#include "../entry/audioplugin.h"
#include "../ext/clap/include/clap/events.h"




//
//Experimental API

namespace Reflex::System
{

	struct ClapEventsProperty;	//Cast<ClapEventsProperty>(QueryAbstractProperty(instance, K32("clap_events_extension")))

}




//
//ClapEventsProperty

struct Reflex::System::ClapEventsProperty : public Object
{
	static constexpr UInt8 kTypeClap = 2;			//Use the third AudioPlugin::Event type slot (numeric value 2) for raw CLAP output

	bool supports_native_note_input = false;		//Advertise native CLAP notes and expressions on input note ports.
	bool supports_native_note_output = false;		//Advertise native CLAP notes and expressions on output note ports.

	const clap_input_events * input = nullptr;		//Complete host input list; valid only during the Rt process callback.

	Array <UInt8> output;							//Preallocated by the adapter (16kb) and cleared before each block
};

//Query this extension in the client constructor and set the support options there only
//Clients may reserve additional output capacity before real-time processing begins.
//Input MIDI and parameter-value events also arrive through the standard input events.
//Append raw bytes to output, then add a kTypeClap entry to the regular events_out array.
//Set idx to the byte offset and position to the sample offset (header.time).
//Keep events_out ordered by Event::position.
