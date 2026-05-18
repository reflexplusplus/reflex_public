#pragma once

#include "types.h"




//
//Experimental API

namespace Reflex::System
{

	class AudioDevice;

}




//
//AudioDevice

class Reflex::System::AudioDevice : public Object
{
public:

	REFLEX_OBJECT(System::AudioDevice, Object);

	static AudioDevice & null;

	enum Type : UInt8
	{
		kTypeInput,
		kTypeOutput,
		kTypeDuplex,
	};

	struct Desc
	{
		Array <UInt8> id;
		WString name;

		Type type;
		bool is_default;
	};

	struct Config
	{
		Float sample_rate = 48000;
		UInt16 buffer_size = 1024;
		UInt16 num_input = 0;
		UInt16 num_output = 2;
		bool exclusive = false;

		FunctionPointer <void(Object & owner, UInt samples, const Float32 ** inputs, Float32 ** outputs)> planar_32bit = nullptr;
		FunctionPointer <void(Object & owner, UInt samples, const Float * inputs, Float * outputs)> interleaved_32bit = nullptr;
		FunctionPointer <void(Object & owner, UInt samples, const Int16 * inputs, Int16 * outputs)> interleaved_16bit = nullptr;
	};


	static Array <Desc> GetAvailableDevices();

	static TRef <AudioDevice> Create(Type type, const Array <UInt8> & id);


	virtual ArrayView <UInt8> GetID() const = 0;

	virtual Array <Float> GetAvailableSampleRates() const = 0;

	virtual Array <UInt32> GetAvailableBufferSizes() const = 0;


	virtual bool Init(Object & owner, const Config & config) = 0;

	virtual Float GetSampleRate() const = 0;

	virtual UInt32 GetBufferSize() const = 0;


	virtual bool Start() = 0;

	virtual void Stop() = 0;

};
