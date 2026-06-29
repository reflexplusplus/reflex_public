#pragma once

#include "audio.h"




//
//Experimental API (AudioPlugin is the current official API)

namespace Reflex::System
{

	class Midi;

}




//
//Midi

class Reflex::System::Midi : public Object
{
public:

	REFLEX_OBJECT(System::Midi, Object);

	static Midi & null;


	//types

	class Input;

	class Output;

	using InputCallback = FunctionPointer <void(void * client, Float64 timestamp, ArrayView <UInt8> msg)>;


	//lifetime

	static TRef <Midi> Create();


	//info

	virtual Array <MediaDeviceDesc> GetAvailableDevices(UInt8 direction_flags) const = 0;


	//endpoints

	virtual TRef <Input> CreateInput(ArrayView <UInt8> id, void * client, InputCallback callback) = 0;

	virtual TRef <Output> CreateOutput(ArrayView <UInt8> id) = 0;
};




//
//Midi::Input

class Reflex::System::Midi::Input : public Object
{
public:

	REFLEX_OBJECT(System::Midi::Input, Object);

	static Input & null;

	using Callback = InputCallback;

};




//
//Midi::Output

class Reflex::System::Midi::Output : public Object
{
public:

	REFLEX_OBJECT(System::Midi::Output, Object);

	static Output & null;


	virtual bool Send(ArrayView <UInt8> msg) = 0;

};