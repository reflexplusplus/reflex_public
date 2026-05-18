#pragma once

#include "../[require].h"

#if defined(REFLEX_OS_WINDOWS)
	#if REFLEX_64BIT
		#pragma pack(push, 16)
	#else
		#pragma pack(push, 8)
	#endif
#elif defined(REFLEX_OS_MACOS)
	#if REFLEX_64BIT
		#pragma pack(push, 16)
	#else
		#pragma pack(push, 1)
	#endif
#elif defined(REFLEX_OS_LINUX)
	#if REFLEX_64BIT
		#pragma pack(push, 16)
	#else
		#pragma pack(push, 8)
	#endif
#elif (defined(REFLEX_OS_IOS) || defined(REFLEX_OS_ANDROID))
	//placeholder for compilation
	#if REFLEX_64BIT
		#pragma pack(push, 16)
	#else
		#pragma pack(push, 8)
	#endif
#else
	REFLEX_STATIC_ASSERT(false);
#endif

#define REFLEX_DECLARE_VST3_CLASS(TYPE, BASE, uid0, uid1, uid2, uid3) template <> struct ClassInfo <TYPE> {static inline const UID iid = MakeUID(uid0, uid1, uid2, uid3);}; template <> struct BaseClass <TYPE> { using Type = BASE; }




//
//Internal

namespace Reflex::VST3API
{

	using UID = Tuple <UInt32, UInt32, UInt32, UInt32>; // plain UID type
	using ParamID = UInt32;

	inline UID MakeUID(UInt32 a, UInt32 b, UInt32 c, UInt32 d)
	{
		#if defined(REFLEX_OS_WINDOWS)
			auto t = Reinterpret<Pair<UInt16>>(b);
			return MakeTuple(a, Reinterpret<UInt32>(MakeTuple(t.b, t.a)), BitReverse(c), BitReverse(d));
		#elif (defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_LINUX))
			return MakeTuple(BitReverse(a), BitReverse(b), BitReverse(c), BitReverse(d));
		#elif (defined(REFLEX_OS_IOS) || defined(REFLEX_OS_ANDROID))
			//placeholder for compilation
			return MakeTuple(BitReverse(a), BitReverse(b), BitReverse(c), BitReverse(d));
		#else
			REFLEX_STATIC_ASSERT(false);
		#endif
	}

	typedef char16_t String128[128];

	template <class CLASS>
	struct ClassInfo
	{
	};

	enum tresult : UInt32
	{
		kResultOk = 0,
		kResultFalse = 1,
		#if defined(REFLEX_OS_WINDOWS)
		kResultNotImplemented = 0x80004001L,
		kResultNoInterface = 0x80004002L,
		#else
		kResultNotImplemented = 3,
		kResultNoInterface = UInt32(-1),
		#endif
	};



	//declarations

	static constexpr const char * kAudioEffectClass = "Audio Module Class";
	static constexpr const char * kComponentControllerClass = "Component Controller Class";

	struct IUnknown;
	struct IBStream;

	struct IPluginFactory;
	struct IPluginFactory2;
	struct IPluginBase;

	struct IPluginComponent;
	struct IPluginAudioProcessor;
	struct IPluginEditController;
	struct IPluginMidiMapping;
	struct IPluginView;
	struct IAttributeList;
	struct IMessage;
	struct IConnectionPoint;

	struct IHostApplication;
	struct IHostAutomation;
	struct IHostView;

	struct IParamValueQueue;
	struct IParameterChanges;
	struct IEventList;



	//structs

	struct PFactoryInfo
	{
		enum Flags
		{
			kClassesDiscardable = 1 << 0,	// The number of exported classes can change each time the Module is loaded
			kComponentNonDiscardable = 1 << 3,	// Component won't be unloaded until process exit
			kUnicode = 1 << 4
		};

		char vendor[64];
		char url[256] = { 0 };
		char email[128] = { 0 };
		Int32 flags = kUnicode;
	};

	struct PClassInfo
	{
		UID cid;
		Int32 cardinality = 0x7FFFFFFF;
		char category[32] = { 0 };
		char name[64] = { 0 };
	};

	struct PClassInfo2
	{
		PClassInfo pclassinfo = {};
		UInt32 classFlags = 0;
		char subcategory[128] = { 0 };
		char vendor[64] = { 0 };
		char version[64] = "1.0.0.1";
		char sdkVersion[64] = "VST 3.6.14";
	};

	enum MediaType
	{
		kMediaTypeAudio = 0,
		kMediaTypeEvent = 1
	};

	enum Scope
	{
		kScopeInput = 0,
		kScopeOutput = 1
	};

	enum SymbolicSampleSize
	{
		kSymbolicSampleSize32 = 0
	};

	enum ProcessMode
	{
		kProcessModeRealtime = 0
	};

	enum RestartFlags
	{
		kReloadComponent = 1 << 0,	// The Component should be reloaded             [SDK 3.0.0]
		kIoChanged = 1 << 1,	// Input and/or Output Bus configuration has changed        [SDK 3.0.0]
		kParamValuesChanged = 1 << 2,	// Multiple parameter values have changed
		kLatencyChanged = 1 << 3,	// Latency has changed (IAudioProcessor.getLatencySamples)  [SDK 3.0.0]
		kParamTitlesChanged = 1 << 4,	// Parameter titles or default values or flags have changed [SDK 3.0.0]
		kMidiCCAssignmentChanged = 1 << 5,	// MIDI Controller Assignments have changed     [SDK 3.0.1]
		kNoteExpressionChanged = 1 << 6,	// Note Expression has changed (info, count, PhysicalUIMapping, ...) [SDK 3.5.0]
		kIoTitlesChanged = 1 << 7,	// Input and/or Output bus titles have changed  [SDK 3.5.0]
		kPrefetchableSupportChanged = 1 << 8,	// Prefetch support has changed (\see IPrefetchableSupport) [SDK 3.6.1]
		kRoutingInfoChanged = 1 << 9	// RoutingInfo has changed (\see IComponent)    [SDK 3.6.6]
	};

	struct RoutingInfo
	{
		Int32 event;
		Int32 busIndex;
		Int32 channel;
	};

	struct BusInfo
	{
		enum Flags
		{
			kDefaultActive = 1 << 0 // The Plug-in wants that this bus should be activated
			///(activateBus call is requested), by default a bus is inactive
		};

		Int32 event;	// Media type - has to be a value of \ref MediaTypes
		Int32 direction; // input or output \ref BusDirections
		Int32 channelCount;		// number of channels (if used then need to be recheck after \ref
		String128 name;			// name of the bus
		Int32 aux;		// main or aux - has to be a value of \ref BusTypes
		UInt32 flags;
	};

	struct AudioBusBuffers
	{
		Int32 numChannels;		// number of audio channels in bus
		UInt64 silenceFlags;	// Bitset of silence state per channel
		union
		{
			Float32 ** channelBuffers32;	// sample buffers to process with 32-bit precision
			Float64 ** channelBuffers64;	// sample buffers to process with 64-bit precision
		};
	};

	struct ProcessSetup
	{
		Int32 processMode;			// \ref ProcessModes
		Int32 symbolicSampleSize;	// \ref SymbolicSampleSizes
		Int32 maxSamplesPerBlock;	// maximum number of samples per audio block
		Float64 sampleRate;		// sample rate
	};

	struct ProcessContext
	{
		enum StatesAndFlags
		{
			kPlaying = 1 << 1,		// currently playing
			kCycleActive = 1 << 2,		// cycle is active
			kRecording = 1 << 3,		// currently recording
			kSystemTimeValid = 1 << 8,		// systemTime contains valid information
			kContTimeValid = 1 << 17,	// continousTimeSamples contains valid information
			kProjectTimeMusicValid = 1 << 9,// projectTimeMusic contains valid information
			kBarPositionValid = 1 << 11,	// barPositionMusic contains valid information
			kCycleValid = 1 << 12,	// cycleStartMusic and barPositionMusic contain valid information
			kTempoValid = 1 << 10,	// tempo contains valid information
			kTimeSigValid = 1 << 13,	// timeSigNumerator and timeSigDenominator contain valid information
			kChordValid = 1 << 18,	// chord contains valid information
			kSmpteValid = 1 << 14,	// smpteOffset and frameRate contain valid information
			kClockValid = 1 << 15		// samplesToNextClock valid
		};

		UInt32 state = 0;
		Float64 sampleRate = 44100.0f;
		Int64 projectTimeSamples = 0;
		Int64 systemTime = 0;
		Int64 continousTimeSamples = 0;
		Float64 projectTimeMusic = 0;	// musical position in quarter notes (1.0 equals 1 quarter note)
		Float64 barPositionMusic = 0;	// last bar start position, in quarter notes
		Float64 cycleStartMusic = 0;	// cycle start in quarter notes
		Float64 cycleEndMusic = 0;	// cycle end in quarter notes
		Float64 tempo = 120.0;
		Pair <Int32> timeSignature = { 4, 4 };
		Tuple <UInt8, UInt8, Int16> chord;
		Int32 smpteOffsetSubframes = 0;
		Pair <UInt32> frameRate;
		Int32 samplesToNextClock = 0;
	};

	struct ProcessData
	{
		Int32 processMode;			///0 == realtime, 1 == prefetch, 2 == offline
		Int32 symbolicSampleSize;   // sample size - value of \ref SymbolicSampleSizes
		Int32 numSamples;			// number of samples to process
		Int32 numInputs;			// number of audio input buses
		Int32 numOutputs;			// number of audio output buses
		AudioBusBuffers * inputs;	// buffers of input buses
		AudioBusBuffers * outputs;	// buffers of output buses
		IParameterChanges * inputParameterChanges;	// incoming parameter changes for this block
		IParameterChanges * outputParameterChanges;	// outgoing parameter changes for this block (optional)
		IEventList * inputEvents;				// incoming events for this block (optional)
		IEventList * outputEvents;				// outgoing events for this block (optional)
		ProcessContext * processContext;			// processing context (optional, but most welcome)
	};

	struct ParameterInfo
	{
		UInt32 id;				// unique identifier of this parameter (named tag too)
		String128 title;		// parameter title (e.g. "Volume")
		String128 shortTitle;	// parameter shortTitle (e.g. "Vol")
		String128 units;		// parameter unit (e.g. "dB")
		Int32 stepCount;		// number of discrete steps (0: continuous, 1: toggle, discrete value otherwise
		Float64 defaultNormalizedValue;	// default normalized value [0,1] (in case of discrete value: defaultNormalizedValue = defDiscreteValue / stepCount)
		Int32 unitId;			// id of unit this parameter belongs to (see \ref vst3UnitsIntro)
		Int32 flags;			// ParameterFlags (see below)

		enum ParameterFlags
		{
			kCanAutomate = 1 << 0,	// parameter can be automated
			kIsList = 1 << 3,	// parameter should be displayed as list in generic editor or automation editing [SDK 3.1.0]
			kIsProgramChange = 1 << 15,
		};
	};

	struct Event
	{
		enum Flags
		{
			kIsLive = 1 << 0,			// indicates that the event is played live (directly from keyboard)
			kUserReserved1 = 1 << 14,	// reserved for user (for internal use)
			kUserReserved2 = 1 << 15	// reserved for user (for internal use)
		};

		enum Type
		{
			kTypeNoteOn = 0,
			kTypeNoteOff = 1,
			kTypePolyPressure = 3,
			kTypeLegacyMIDICCOut = 65535,

			kTypeData = 2,
			kTypeNoteExpressionValue = 4,
			kTypeNoteExpressionText = 5,
			kTypeChord = 6,
			kTypeScale = 7,
		};

		Int32 busIndex;				// event bus index
		Int32 sampleOffset;			// sample frames related to the current block start sample position
		Float64 ppqPosition;	// position in project
		UInt16 flags;				// combination of \ref EventFlags
		UInt16 type;				// a value from \ref EventTypes

		union
		{
			struct { Int16 channel, pitch; Float32 tuning, velocity; Int32 length, noteId; } noteOn;
			struct { Int16 channel, pitch; Float32 velocity; Int32 noteId; Float32 tuning; } noteOff;
			struct { Int16 channel, pitch; Float32 pressure; Int32 noteId; } polyPressure;
			struct { UInt8 controlNumber; Int8 channel; Int8 value; Int8 value2; } midiCCOut;

			struct { UInt32 size; UInt32 type; const UInt8 * bytes; } data;						//size,type,bytes;
#if (defined(REFLEX_OS_WINDOWS) && !REFLEX_64BIT)
#pragma pack(push, 4)
			struct { UInt32 type; Int32 noteid; Float64 value; } noteExpressionValue;	// type == kNoteExpressionValueEvent
#pragma pack(pop)
#else
			struct { UInt32 type; Int32 noteid; Float64 value; } noteExpressionValue;	// type == kNoteExpressionValueEvent
#endif

			struct { UInt32 type; Int32 noteid; UInt32 textlen; const char16_t * text; } noteExpressionText;		// type == kNoteExpressionTextEvent
			struct { Int16 root, bassnote; Int16 mask; UInt16 textlen; const char16_t * text; } chord;								// type == kChordEvent
			struct { Int16 root, mask;  UInt16 textlen; const char16_t * text; } scale;
		};
	};

	struct ViewRect
	{
		Int32 left, top, right, bottom;
	};

	enum MidiControllers
	{
		kCtrlBankSelectMSB = 0,
		kCtrlModWheel = 1,
		kCtrlBreath = 2,
		kCtrlFoot = 4,
		kCtrlPortaTime = 5,
		kCtrlDataEntryMSB = 6,
		kCtrlVolume = 7,
		kCtrlBalance = 8,

		kCtrlPan = 10,
		kCtrlExpression = 11,
		kCtrlEffect1 = 12,
		kCtrlEffect2 = 13,

		kCtrlGPC1 = 16,
		kCtrlGPC2 = 17,
		kCtrlGPC3 = 18,
		kCtrlGPC4 = 19,

		kCtrlBankSelectLSB = 32,

		kCtrlDataEntryLSB = 38,

		kCtrlSustainOnOff = 64,
		kCtrlPortaOnOff = 65,
		kCtrlSustenutoOnOff = 66,
		kCtrlSoftPedalOnOff = 67,
		kCtrlLegatoFootSwOnOff = 68,
		kCtrlHold2OnOff = 69,

		kCtrlSoundVariation = 70,
		kCtrlFilterCutoff = 71,
		kCtrlReleaseTime = 72,
		kCtrlAttackTime = 73,
		kCtrlFilterResonance = 74,
		kCtrlDecayTime = 75,
		kCtrlVibratoRate = 76,
		kCtrlVibratoDepth = 77,
		kCtrlVibratoDelay = 78,
		kCtrlSoundCtrler10 = 79,

		kCtrlGPC5 = 80,
		kCtrlGPC6 = 81,
		kCtrlGPC7 = 82,
		kCtrlGPC8 = 83,

		kCtrlPortaControl = 84,

		kCtrlEff1Depth = 91,
		kCtrlEff2Depth = 92,
		kCtrlEff3Depth = 93,
		kCtrlEff4Depth = 94,
		kCtrlEff5Depth = 95,

		kCtrlDataIncrement = 96,
		kCtrlDataDecrement = 97,
		kCtrlNRPNSelectLSB = 98,
		kCtrlNRPNSelectMSB = 99,
		kCtrlRPNSelectLSB = 100,
		kCtrlRPNSelectMSB = 101,

		kCtrlAllSoundsOff = 120,
		kCtrlResetAllCtrlers = 121,
		kCtrlLocalCtrlOnOff = 122,
		kCtrlAllNotesOff = 123,
		kCtrlOmniModeOff = 124,
		kCtrlOmniModeOn = 125,
		kCtrlPolyModeOnOff = 126,
		kCtrlPolyModeOn = 127,

		kCtrlAfterTouch = 128,
		kCtrlPitchBend = 129,

		//for kLegacyMIDICCOutEvent
		//kCtrlProgramChange = 130,
		//kCtrlPolyPressure = 131,
	};




	//interfaces

	struct IUnknown
	{
		virtual tresult REFLEX_STDCALL queryInterface(const UID & iid, IUnknown * & obj) = 0;
		virtual UInt32 REFLEX_STDCALL addRef() = 0;
		virtual UInt32 REFLEX_STDCALL release() = 0;
	};

	struct IBStream : public IUnknown
	{
		enum SeekMode
		{
			kSeekModeSet,
			kSeekModeCur,
			kSeekModeEnd
		};

		virtual tresult REFLEX_STDCALL read(void * buffer, Int32 numBytes, Int32 * numBytesRead = 0) = 0;
		virtual tresult REFLEX_STDCALL write(void * buffer, Int32 numBytes, Int32 * numBytesWritten = 0) = 0;
		virtual tresult REFLEX_STDCALL seek(Int64 pos, Int32 mode, Int64 * result = 0) = 0;
		virtual tresult REFLEX_STDCALL tell(Int64 * pos) = 0;
	};

	struct IPluginFactory : public IUnknown
	{
		virtual tresult REFLEX_STDCALL getFactoryInfo(PFactoryInfo & info) = 0;
		virtual Int32 REFLEX_STDCALL countClasses() = 0;
		virtual tresult REFLEX_STDCALL getClassInfo(Int32 index, PClassInfo & info) = 0;
		virtual tresult REFLEX_STDCALL createInstance(const UID & cid, const UID & iid, IUnknown * & obj) = 0;
	};

	struct IPluginFactory2 : public IPluginFactory
	{
		virtual tresult REFLEX_STDCALL getClassInfo2(Int32 index, PClassInfo2 & info) = 0;
	};

	struct IPluginBase : public IUnknown
	{
		virtual tresult REFLEX_STDCALL initialize(IUnknown * context) = 0;
		virtual tresult REFLEX_STDCALL terminate() = 0;
	};

	struct IPluginComponent : public IPluginBase
	{
		virtual tresult REFLEX_STDCALL getControllerClassId(UID & classId) = 0;
		virtual tresult REFLEX_STDCALL setIoMode(Int32 iomode) = 0;
		virtual Int32 REFLEX_STDCALL getBusCount(Int32 mediatype, Int32 busdirection) = 0;
		virtual tresult REFLEX_STDCALL getBusInfo(Int32 mediatype, Int32 busdirection, Int32 index, BusInfo & bus_out) = 0;
		virtual tresult REFLEX_STDCALL getRoutingInfo(RoutingInfo & inInfo, RoutingInfo & outInfo_out) = 0;
		virtual tresult REFLEX_STDCALL activateBus(Int32 mediatype, Int32 busdirection, Int32 index, UInt8 state) = 0;
		virtual tresult REFLEX_STDCALL setActive(UInt8 state) = 0;
		virtual tresult REFLEX_STDCALL setState(IBStream * state) = 0;
		virtual tresult REFLEX_STDCALL getState(IBStream * state) = 0;
	};

	struct IPluginAudioProcessor : public IUnknown
	{
		virtual tresult REFLEX_STDCALL setBusArrangements(UInt64 * input_speakerarrangements, Int32 numIns, UInt64 * output_speakerarrangements, Int32 numOuts) = 0;
		virtual tresult REFLEX_STDCALL getBusArrangement(Int32 busdirection, Int32 index, UInt64 & speakerarrangement) = 0;
		virtual tresult REFLEX_STDCALL canProcessSampleSize(Int32 symbolicSampleSize) = 0;
		virtual UInt32 REFLEX_STDCALL getLatencySamples() = 0;
		virtual tresult REFLEX_STDCALL setupProcessing(ProcessSetup & setup) = 0;
		virtual tresult REFLEX_STDCALL setProcessing(UInt8 state) = 0;
		virtual tresult REFLEX_STDCALL process(ProcessData & data) = 0;
		virtual UInt32 REFLEX_STDCALL getTailSamples() = 0;
	};

	struct IPluginEditController : public IPluginBase
	{
		static constexpr const char * kEditor = "editor";

		virtual tresult REFLEX_STDCALL setComponentState(IBStream * state) = 0;
		virtual tresult REFLEX_STDCALL setState(IBStream * state) = 0;
		virtual tresult REFLEX_STDCALL getState(IBStream * state) = 0;
		virtual Int32 REFLEX_STDCALL getParameterCount() = 0;
		virtual tresult REFLEX_STDCALL getParameterInfo(Int32 paramIndex, ParameterInfo & info_out) = 0;
		virtual tresult REFLEX_STDCALL getParamStringByValue(UInt32 paramid, Float64 valueNormalized /*in*/, String128 string_out) = 0;
		virtual tresult REFLEX_STDCALL getParamValueByString(UInt32 paramid, char16_t * string /*in*/, Float64 & valueNormalized_out) = 0;

		virtual Float64 REFLEX_STDCALL normalizedParamToPlain(UInt32 paramid, Float64 valueNormalized) = 0;
		virtual Float64 REFLEX_STDCALL plainParamToNormalized(UInt32 paramid, Float64 plainValue) = 0;
		virtual Float64 REFLEX_STDCALL getParamNormalized(UInt32 paramid) = 0;
		virtual tresult REFLEX_STDCALL setParamNormalized(UInt32 paramid, Float64 value) = 0;
		virtual tresult REFLEX_STDCALL setComponentHandler(IHostAutomation * handler) = 0;
		virtual IPluginView * REFLEX_STDCALL createView(const char * name) = 0;
	};

	struct IPluginMidiMapping : public IUnknown
	{
		virtual tresult REFLEX_STDCALL getMidiControllerAssignment(Int32 busIndex, Int16 channel, Int16 cc, UInt32 & id) = 0;
	};

	struct IPluginView : public IUnknown
	{
		static constexpr const char * kPlatformTypeHWND = "HWND";
		static constexpr const char * kPlatformTypeNSView = "NSView";

		virtual tresult REFLEX_STDCALL isPlatformTypeSupported(const char * type) = 0;
		virtual tresult REFLEX_STDCALL attached(void * parent, const char * type) = 0;
		virtual tresult REFLEX_STDCALL removed() = 0;
		virtual tresult REFLEX_STDCALL onWheel(float distance) = 0;
		virtual tresult REFLEX_STDCALL onKeyDown(char16_t key, Int16 keyCode, Int16 modifiers) = 0;
		virtual tresult REFLEX_STDCALL onKeyUp(char16_t key, Int16 keyCode, Int16 modifiers) = 0;
		virtual tresult REFLEX_STDCALL getSize(ViewRect * size) = 0;
		virtual tresult REFLEX_STDCALL onSize(/*const*/ ViewRect * newSize) = 0;
		virtual tresult REFLEX_STDCALL onFocus(UInt8 state) = 0;
		virtual tresult REFLEX_STDCALL setFrame(IHostView * frame) = 0;
		virtual tresult REFLEX_STDCALL canResize() = 0;
		virtual tresult REFLEX_STDCALL checkSizeConstraint(ViewRect * rect) = 0;
	};

	struct IAttributeList : public IUnknown
	{
		virtual tresult REFLEX_STDCALL setInt(const char * id, Int64 value) = 0;
		virtual tresult REFLEX_STDCALL getInt(const char * id, Int64 & value) = 0;
		virtual tresult REFLEX_STDCALL setFloat(const char * id, Float64 value) = 0;
		virtual tresult REFLEX_STDCALL getFloat(const char * id, Float64 & value) = 0;
		virtual tresult REFLEX_STDCALL setString(const char * id, const char16_t * string) = 0;
		virtual tresult REFLEX_STDCALL getString(const char * id, char16_t * string, UInt32 sizeInBytes) = 0;
		virtual tresult REFLEX_STDCALL setBinary(const char * id, const void * data, UInt32 sizeInBytes) = 0;
		virtual tresult REFLEX_STDCALL getBinary(const char * id, const void * & data, UInt32 & sizeInBytes) = 0;
	};

	struct IMessage : public IUnknown
	{
		virtual const char * REFLEX_STDCALL getMessageID() = 0;
		virtual void REFLEX_STDCALL setMessageID(const char * id) = 0;
		virtual IAttributeList * REFLEX_STDCALL getAttributes() = 0;
	};

	struct IConnectionPoint : public IUnknown
	{
		virtual tresult REFLEX_STDCALL connect(IConnectionPoint * other) = 0;
		virtual tresult REFLEX_STDCALL disconnect(IConnectionPoint * other) = 0;
		virtual tresult REFLEX_STDCALL notify(IMessage * message) = 0;
	};



	//host

	struct IHostApplication : public IUnknown
	{
		virtual tresult REFLEX_STDCALL getName(String128 name) = 0;
		virtual tresult REFLEX_STDCALL createInstance(UID & cid, UID & iid, IUnknown *& obj) = 0;
	};

	struct IHostAutomation : public IUnknown
	{
		virtual tresult REFLEX_STDCALL beginEdit(UInt32 paramid) = 0;
		virtual tresult REFLEX_STDCALL performEdit(UInt32 paramid, Float64 valueNormalized) = 0;
		virtual tresult REFLEX_STDCALL endEdit(UInt32 paramid) = 0;
		virtual tresult REFLEX_STDCALL restartComponent(Int32 flags) = 0;
	};

	struct IHostView : public IUnknown
	{
		virtual tresult REFLEX_STDCALL resizeView(IPluginView * view, ViewRect & size) = 0;
	};



	//common

	struct IParamValueQueue : public IUnknown
	{
		virtual UInt32 REFLEX_STDCALL getParameterId() = 0;
		virtual Int32 REFLEX_STDCALL getPointCount() = 0;
		virtual tresult REFLEX_STDCALL getPoint(Int32 index, Int32 & sampleOffset_out, Float64 & value_out) = 0;
		virtual tresult REFLEX_STDCALL addPoint(Int32 sampleOffset, Float64 value, Int32 & index_out) = 0;
	};

	struct IParameterChanges : public IUnknown
	{
		virtual Int32 REFLEX_STDCALL getParameterCount() = 0;
		virtual IParamValueQueue * REFLEX_STDCALL getParameterData(Int32 index) = 0;
		virtual IParamValueQueue * REFLEX_STDCALL addParameterData(const UInt32 & paramid, Int32 & index_out) = 0;
	};

	struct IEventList : public IUnknown
	{
		virtual Int32 REFLEX_STDCALL getEventCount() = 0;
		virtual tresult REFLEX_STDCALL getEvent(Int32 index, Event & e_out) = 0;
		virtual tresult REFLEX_STDCALL addEvent(Event & e /*in*/) = 0;
	};

	struct IUnitInfo : public IUnknown
	{
		struct UnitInfo
		{
			Int32 id;						///< unit identifier
			Int32 parentUnitId;			///< identifier of parent unit (kNoParentUnitId: does not apply, this unit is the root)
			String128 name;					///< name, optional for the root component, required otherwise
			Int32 programListId;	///< id of program list used in unit (kNoProgramListId = no programs used in this unit)
		};

		struct ProgramListInfo
		{
			Int32 id;				///< program list identifier
			String128 name;					///< name of program list
			Int32 programCount;				///< number of programs in this list
		};

		virtual Int32 REFLEX_STDCALL getUnitCount() = 0;
		virtual tresult REFLEX_STDCALL getUnitInfo(Int32 unitIndex, UnitInfo & info_out) = 0;
		virtual Int32 REFLEX_STDCALL getProgramListCount() = 0;
		virtual tresult REFLEX_STDCALL getProgramListInfo(Int32 listIndex, ProgramListInfo & info_out) = 0;
		virtual tresult REFLEX_STDCALL getProgramName(Int32 listId, Int32 programIndex, String128 name_out) = 0;
		virtual tresult REFLEX_STDCALL getProgramInfo(Int32 listId, Int32 programIndex, const char * attributeId_in, String128 attributeValue_out) = 0;
		virtual tresult REFLEX_STDCALL hasProgramPitchNames(Int32 listId, Int32 programIndex) = 0;
		virtual tresult REFLEX_STDCALL getProgramPitchName(Int32 listId, Int32 programIndex, Int16 midiPitch, String128 name_out) = 0;
		virtual Int32 REFLEX_STDCALL getSelectedUnit() = 0;
		virtual tresult REFLEX_STDCALL selectUnit(Int32 unitId) = 0;
		virtual tresult REFLEX_STDCALL getUnitByBus(Int32 evnt, Int32 output, Int32 idx, Int32 channel, Int32& unitId_out) = 0;
		virtual tresult REFLEX_STDCALL setUnitProgramData(Int32 listOrUnitId, Int32 programIndex, IBStream* data) = 0;
	};

	template <class CLASS>
	struct BaseClass
	{
		typedef void Type;
	};

	REFLEX_DECLARE_VST3_CLASS(IUnknown, void, 0x00000000, 0x00000000, 0xC0000000, 0x00000046);
	REFLEX_DECLARE_VST3_CLASS(IBStream, IUnknown, 0xC3BF6EA2, 0x30994752, 0x9B6BF990, 0x1EE33E9B);
	REFLEX_DECLARE_VST3_CLASS(IPluginBase, IUnknown, 0x22888DDB, 0x156E45AE, 0x8358B348, 0x08190625);

	REFLEX_DECLARE_VST3_CLASS(IPluginFactory, IUnknown, 0x7A4D811C, 0x52114A1F, 0xAED9D2EE, 0x0B43BF9F);
	REFLEX_DECLARE_VST3_CLASS(IPluginFactory2, IPluginFactory, 0x0007B650, 0xF24B4C0B, 0xA464EDB9, 0xF00B2ABB);
	REFLEX_DECLARE_VST3_CLASS(IPluginComponent, IPluginBase, 0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802);
	REFLEX_DECLARE_VST3_CLASS(IPluginAudioProcessor, IUnknown, 0x42043F99, 0xB7DA453C, 0xA569E79D, 0x9AAEC33D);
	REFLEX_DECLARE_VST3_CLASS(IPluginEditController, IPluginBase, 0xDCD7BBE3, 0x7742448D, 0xA874AACC, 0x979C759E);
	REFLEX_DECLARE_VST3_CLASS(IPluginMidiMapping, IUnknown, 0xDF0FF9F7, 0x49B74669, 0xB63AB732, 0x7ADBF5E5);
	REFLEX_DECLARE_VST3_CLASS(IPluginView, IUnknown, 0x5BC32507, 0xD06049EA, 0xA6151B52, 0x2B755B29);
	REFLEX_DECLARE_VST3_CLASS(IAttributeList, IUnknown, 0x1E5F0AEB, 0xCC7F4533, 0xA2544011, 0x38AD5EE4);
	REFLEX_DECLARE_VST3_CLASS(IMessage, IUnknown, 0x936F033B, 0xC6C047DB, 0xBB0882F8, 0x13C1E613);
	REFLEX_DECLARE_VST3_CLASS(IConnectionPoint, IUnknown, 0x70A4156F, 0x6E6E4026, 0x989148BF, 0xAA60D8D1);

	REFLEX_DECLARE_VST3_CLASS(IHostAutomation, IUnknown, 0x93A0BEA3, 0x0BD045DB, 0x8E890B0C, 0xC1E46AC6);
	REFLEX_DECLARE_VST3_CLASS(IHostApplication, IUnknown, 0x58E595CC, 0xDB2D4969, 0x8B6AAF8C, 0x36A664E5);

	REFLEX_DECLARE_VST3_CLASS(IParamValueQueue, IUnknown, 0x01263A18, 0xED074F6F, 0x98C9D356, 0x4686F9BA);
	REFLEX_DECLARE_VST3_CLASS(IParameterChanges, IUnknown, 0xA4779663, 0x0BB64A56, 0xB44384A8, 0x466FEB9D);
	REFLEX_DECLARE_VST3_CLASS(IEventList, IUnknown, 0x3A2C4214, 0x346349FE, 0xB2C4F397, 0xB9695A44);

	REFLEX_DECLARE_VST3_CLASS(IUnitInfo, IUnknown, 0x3D4BD6B5, 0x913A4FD2, 0xA886E768, 0xA5EB92C1);


	typedef IPluginFactory * (REFLEX_STDCALL * EntryFn)();

	constexpr CString::View kEntryFnName = "GetPluginFactory";

}

#pragma pack(pop)




//verify api

#define REFLEX_VST3_VERIFY (__has_include("pluginterfaces/base/ipluginbase.h"))

#if REFLEX_VST3_VERIFY
#define GetPluginFactory HIDE
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstunits.h"
#undef GetPluginFactory
#endif

#if REFLEX_VST3_VERIFY

#define VERIFY_ENUM(x,y) REFLEX_STATIC_ASSERT(int(Reflex::VST3API::x) == int(Steinberg::y))
#define VERIFY_X(x,y) REFLEX_STATIC_ASSERT(sizeof(Reflex::VST3API::x) == sizeof(Steinberg::y))
#define VERIFY(x) VERIFY_X(x, x)
#define VERIFY_VST(x) VERIFY_X(x, Vst::x)
#define VERIFY_SUBCLASS(cls,x) VERIFY_X(cls::x, Vst::x)

VERIFY_ENUM(kResultOk, kResultOk);
VERIFY_ENUM(kResultFalse, kResultFalse);
VERIFY_ENUM(kResultNotImplemented, kNotImplemented);
VERIFY_ENUM(kResultNoInterface, kNoInterface);

VERIFY(PFactoryInfo);
VERIFY(PClassInfo);
VERIFY(PClassInfo2);
VERIFY(ViewRect);
VERIFY_VST(RoutingInfo);
VERIFY_VST(BusInfo);
VERIFY_VST(AudioBusBuffers);
VERIFY_VST(ProcessSetup);
VERIFY_VST(ProcessContext);
VERIFY_VST(ProcessData);
VERIFY_VST(ParameterInfo);
VERIFY_VST(Event);

VERIFY_X(IUnknown, FUnknown);
VERIFY(IBStream);
VERIFY(IPluginBase);
VERIFY(IPluginFactory);
VERIFY(IPluginFactory2);
VERIFY_X(IPluginView, IPlugView);
VERIFY_X(IPluginComponent, Vst::IComponent);
VERIFY_X(IPluginAudioProcessor, Vst::IAudioProcessor);
VERIFY_X(IPluginEditController, Vst::IEditController);
VERIFY_X(IAttributeList, Vst::IAttributeList);
VERIFY_X(IMessage, Vst::IMessage);
VERIFY_X(IConnectionPoint, Vst::IConnectionPoint);

VERIFY_VST(IHostApplication);
VERIFY_X(IHostView, IPlugFrame);
VERIFY_X(IHostAutomation, Vst::IComponentHandler);

#endif

REFLEX_STATIC_ASSERT(sizeof(char16_t) == sizeof(Reflex::UInt16));
