#pragma once

#include "instance.h"




//
//Secondary API

namespace Reflex::System
{

	class AudioPlugin;

}




//
//AudioPlugin

class Reflex::System::AudioPlugin : public App
{
public:

	REFLEX_OBJECT(System::AudioPlugin, App);



	//types

	struct Configuration : public App::Configuration
	{
		struct Class;

		Function <TRef<Object>(Object & global, const Class & cls, AudioPlugin & instance)> instance_ctr;

		Array <Class> classes;
	};

	class Callbacks;

	class Lock;

	struct Event;

	struct EventBuffer;

	enum ChangeFlags : UInt8
	{
		kChangeParameterValues = 1,
		kChangeParameterInfo = 2,
		kChangeState = 4,
	};



	//properties

	static TRef <Object> OnStart(const ArrayView <CString::View> & cmdline, Configuration & config);	//return your app global



	//global

	static CString::View GetPluginHost();

	static WString GetDefaultDevice(bool recording);	//recording flag is used as hint for mobile platforms



	//midi

	virtual Array < Pair <WString,bool> > GetMidiPorts(bool output) const = 0;



	//audio

	virtual Array <WString> GetAvailableAudioDevices() const = 0;

	virtual WString GetCurrentAudioDevice() const = 0;

	virtual UInt32 GetCurrentBufferSize() const = 0;

	virtual Float32 GetCurrentSampleRate() const = 0;


	virtual Array <UInt32> GetAvailableBufferSizes() const = 0;

	virtual Array <Float32> GetAvailableSampleRates() const = 0;


	virtual Array < Pair <WString,bool> > GetAudioChannels(bool output) const = 0;



	//update the host (expensive in most hosts, call strictly when neccesary)

	virtual void ReportProcessingDelay(UInt32 delay) = 0;

	virtual void ReportStateChange(UInt8 change_flags) = 0;	//parameters, descriptions or general state (for parameters, do not call for host automation!)



	//parameters

	virtual void BeginAutomation(UInt32 idx) = 0;

	virtual void Automate(UInt32 idx, Float32 value) = 0;

	virtual void EndAutomation(UInt32 idx) = 0;

};




//
//AudioPlugin::Configuration::Class

struct Reflex::System::AudioPlugin::Configuration::Class
{
	enum Type : UInt32
	{
		kTypeAudioProcessor,
		kTypeAudioGenerator,

		kTypeEventProcessor,
		kTypeEventGenerator,

		kTypeAudioEffect = kTypeAudioProcessor,
		kTypeInstrument = kTypeAudioGenerator,

		kNumType = kTypeEventGenerator + 1
	};

	enum Category : UInt32
	{
		kUncategorised = 0,

		//Effect categories
		kEQ = 1,
		kDynamics = 2,
		kReverb = 4,
		kDelay = 8,
		kModulation = 16,
		kPitchShift = 32,
		kDistortion = 64,
		kFilter = 128,
		kRestoration = 256,
		kMastering = 512,
		kSurround = 1024,
		kAnalyzer = 2048,

		//Instrument categories
		kSynth = 4096,
		kSampler = 8192,
		kDrum = 16384,
	};



	//Common

	CString vendor, product;

	Pair <UInt8> channels_io;

	Pair <bool> midi_io;

	UInt16 nparam = 0;

	Type type = kTypeAudioProcessor;

	Category category = kUncategorised;



	//VST2 specific

	struct VST2
	{
		UInt32 uid;
	}

	vst2 = {};



	//VST3 specific

	struct VST3
	{
		Tuple <UInt64, UInt64> uid;
	}

	vst3 = {};



	//AudioUnit specific (must match plist settings!)

	struct AudioUnit
	{
		UInt32 company_4cc;
		UInt32 uid_4cc;
	}

	audiounit = {};



	//AAX specific

	struct AAX
	{
		UInt32 manufacturer_4cc;
		UInt32 product_4cc;
		UInt32 native_4cc;
		UInt32 audiosuite_4cc;
	}

	aax = {};



	//CLAP specific

	struct CLAP
	{
		CString uid;
		CString version;
	}

	clap = {};

};




//
//AudioPlugin::Callbacks

class Reflex::System::AudioPlugin::Callbacks : public InterfaceOf <Callbacks>
{
public:

	static Callbacks & null;


	virtual Array <UInt8> OnGetPluginChunk() = 0;

	virtual void OnSetPluginChunk(const ArrayView <UInt8> & archive) = 0;


	virtual void OnGetParameterInfo(UInt32 idx, CString & name) const = 0;

	virtual Float32 OnGetParameterValue(UInt32 idx) const = 0;

	virtual void OnSetParameterValue(UInt32 idx, Float32 value) = 0;	//host edits parameter in UI, usually not used


	virtual FunctionPointer <void(Callbacks&,UInt)> OnPrepare(UInt32 max_buffersize, Float32 samplerate, ConstTRef <EventBuffer> events_in, TRef <EventBuffer> events_out, const ArrayView <const Float32*> & inputs, const ArrayView <Float32*> & outputs) = 0;
};




//
//AudioPlugin::Lock

class Reflex::System::AudioPlugin::Lock
{
public:

	//lifetime

	Lock(AudioPlugin & audioplugin);

	~Lock();



	//midi

	void EnableMidiPort(bool output, UInt idx, bool enable);



	//audio

	bool SelectAudioDevice(const WString::View & value);

	void RequestBufferSize(UInt value);

	void RequestSampleRate(Float32 value);

	void EnableAudioChannel(bool output, UInt32 idx, bool enable);



	//links

	const TRef <AudioPlugin> audioplugin;
};




//
//AudioPlugin::MidiEvent

struct Reflex::System::AudioPlugin::Event
{
	enum Type : UInt8
	{
		kTypeMIDI,
		kTypeAutomation,
	};

	UInt16 type : 1;	//0 == MIDI, 1 == Automation

	UInt16 position : 14;

	UInt16 idx;		//MIDI -> portidx, Automation -> parameter

	union { UInt32 u32; Float32 f32; } value;
};




//
//AudioPlugin::EventBuffer

struct Reflex::System::AudioPlugin::EventBuffer
{
	bool clock = false;

	Float64 bps = 1.0;

	Float64 beatpos = 0.0;

	ArrayView <Event> events;
};
