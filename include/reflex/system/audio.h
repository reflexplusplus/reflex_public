#pragma once

#include "types.h"




//
//Experimental API (AudioPlugin is the current official API)

namespace Reflex::System
{

	struct MediaDeviceDesc;

	class Audio;

}




//
//MediaDeviceDesc

struct Reflex::System::MediaDeviceDesc
{
	enum Direction : UInt8
	{
		kDirectionInput = 1 << 0,
		kDirectionOutput = 1 << 1,
	};

	bool operator==(const MediaDeviceDesc&) const = default;

	Array <UInt8> id;

	WString name;

	UInt8 direction_flags = 0;
};




//
//Audio

class Reflex::System::Audio : public Object
{
public:
	
	REFLEX_OBJECT(System::Audio, Object);

	static Audio & null;


	//types

	struct DeviceDesc : public MediaDeviceDesc
	{
		bool is_default;
	};

	class Device;



	//lifetime

	static Array <CString::View> GetAvailablePlatforms();

	static TRef <Audio> Acquire(CString::View platform);



	//info

	virtual CString::View GetPlatformName() const = 0;


	//endpoints

	virtual Array <DeviceDesc> GetAvailableDevices() const = 0;

	virtual TRef <Device> CreateDevice(ArrayView <UInt8> id) = 0;
};




//
//Audio::Device

class Reflex::System::Audio::Device : public Object
{
public:

	REFLEX_OBJECT(System::Audio::Device, Object);

	static Device & null;

	struct Config
	{
		Float sample_rate = 48000;
		UInt16 buffer_size = 1024;
		UInt16 num_input = 0;
		UInt16 num_output = 2;
		bool exclusive = false;

		FunctionPointer <void(void * client, UInt samples, const Float32 ** inputs, Float32 ** outputs)> planar_32bit = nullptr;
		FunctionPointer <void(void * client, UInt samples, const Float * inputs, Float * outputs)> interleaved_32bit = nullptr;
		FunctionPointer <void(void * client, UInt samples, const Int16 * inputs, Int16 * outputs)> interleaved_16bit = nullptr;
	};

	virtual ArrayView <UInt8> GetID() const = 0;
	virtual Array <Float> GetAvailableSampleRates() const = 0;
	virtual Array <UInt32> GetAvailableBufferSizes() const = 0;
	virtual bool Init(void * client, const Config & config) = 0;
	virtual Float GetSampleRate() const = 0;
	virtual UInt32 GetBufferSize() const = 0;
	virtual bool Start() = 0;
	virtual void Stop() = 0;
};