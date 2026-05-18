#pragma once

#include "../[require].h"

#if (defined(REFLEX_OS_WINDOWS) && !REFLEX_64BIT)
#pragma pack(push,8)
#elif defined(__GNUC__)
#pragma pack(push,8)
#endif




//
//Internal

namespace Reflex::VST2API
{

	constexpr Int32 kVersion = 2400;

	constexpr UInt32 kMagic = CC32("VstP");

	struct Plugin;

	typedef	UIntNative(REFLEX_STDCALL * HostCallback)(Plugin & aeffect, Int32 opcode, Int32 index, UIntNative value, void * ptr, Float32 opt);
	typedef UIntNative(REFLEX_STDCALL * PluginCallback)(Plugin & aeffect, Int32 opcode, Int32 index, UIntNative value, void * ptr, Float32 opt);
	typedef void(REFLEX_STDCALL * PluginProcessProc)(Plugin & aeffect, Float32 ** inputs, Float32 ** outputs, Int32 sample_frames);
	typedef void(REFLEX_STDCALL * PluginProcessDoubleProc)(Plugin * effect, Float64 ** inputs, Float64 ** outputs, Int32 sample_frames);
	typedef void(REFLEX_STDCALL * PluginSetParameterCallback)(Plugin & effect, Int32 index, Float32 parameter);
	typedef Float32(REFLEX_STDCALL * PluginGetParameterCallback)(Plugin & effect, Int32 index);
	typedef Plugin*(REFLEX_STDCALL * EntryFn)(HostCallback);

	struct Plugin
	{
		enum Flags
		{
			kFlagHasEditor = 1 << 0,
			kFlagCanReplacing = 1 << 4,
			kFlagProgramChunks = 1 << 5,
			kFlagIsSynth = 1 << 8,
			kFlagNoSoundInStop = 1 << 9,
			kFlagCanDoubleReplacing = 1 << 12,
		};

		UInt32 magic = kMagic;

		PluginCallback dispatcher = 0;
		void * __process_deprecated = 0;
		PluginSetParameterCallback set_parameter = 0;
		PluginGetParameterCallback get_parameter = 0;

		Int32 num_programs = 0;
		Int32 num_params = 0;
		Int32 num_inputs = 0;
		Int32 num_outputs = 0;

		Int32 flags = 0;

		UIntNative reserved_1 = 0;
		UIntNative reserved_2 = 0;

		Int32 initial_delay = 0;

		Int32 __real_qualities_deprecated;
		Int32 __off_qualities_deprecated;
		Float32 __io_ratio_deprecated = 1.0f;

		void * object = 0;
		void * user = 0;

		Int32 unique_id = 0;
		Int32 version = 0;

		PluginProcessProc process_replacing = 0;
		PluginProcessDoubleProc process_double_replacing = 0;
		char future[56];
	};

	enum HostOpcodes
	{
		kHostAutomate = 0,
		kHostVersion,
		kHostCurrentId,
		kHostIdle,
		kHostGetTime = 7,
		kHostProcessEvents,
		kHostIOChanged = 13,
		kHostSizeWindow = 15,
		kHostGetSampleRate,
		kHostGetBlockSize,
		kHostGetInputLatency,
		kHostGetOutputLatency,
		kHostGetAutomationState = 24,
		kHostGetVendorString = 32,
		kHostGetProductString,
		kHostGetVendorVersion,
		kHostVendorSpecific,
		kHostCanDo = 37,
		kHostUpdateDisplay = 42,
		kHostBeginEdit,
		kHostEndEdit,
	};

	enum PluginOpcodes
	{
		kPluginOpen = 0,
		kPluginClose,

		kPluginSetProgram,
		kPluginGetProgram,
		kPluginSetProgramName,
		kPluginGetProgramName,
		kPluginGetParamLabel,
		kPluginGetParamDisplay,
		kPluginGetParamName,
		kPluginSetSampleRate = 10,
		kPluginSetBlockSize,
		kPluginMainsChanged,
		kPluginEditGetRect,
		kPluginEditOpen,
		kPluginEditClose,
		kPluginEditIdle = 19,
		kPluginGetChunk = 23,
		kPluginSetChunk = 24,

		//X
		kPluginProcessEvents = 25,
		kPluginCanBeAutomated,
		kPluginStringToParameter,
		kPluginGetProgramNameIndexed = 29,
		kPluginGetInputProperties = 33,
		kPluginGetOutputProperties,
		kPluginGetPlugCategory,
		kPluginProcessVariableIo = 41,
		kPluginSetSpeakerArrangement,
		kPluginSetBypass = 44,
		kPluginGetEffectName,
		kPluginGetVendorString = 47,
		kPluginGetProductString,
		kPluginGetVendorVersion,
		kPluginVendorSpecific,
		kPluginCanDo,
		kPluginGetTailSize,

		kPluginGetParameterProperties = 57,

		kPluginGetVstVersion = 58,

		//effGetMidiProgramName = 65,
		//effGetCurrentMidiProgram,
		//effGetMidiProgramCategory,
		//effHasMidiProgramsChanged,
		//effGetMidiKeyName,

		kPluginBeginSetProgram = 67,
		kPluginEndSetProgram,

		kPluginGetSpeakerArrangement,
		kPluginShellGetNextPlugin,

		kPluginStartProcess,
		kPluginStopProcess,

		kPluginBeginLoadBank = 75,
		kPluginBeginLoadProgram,

		kPluginGetNumMidiInputChannels = 78,
		kPluginGetNumMidiOutputChannels,
	};

	enum StringConstants
	{
		kMaxProgramNameLength = 24,
		kMaxParameterStringLength = 8,
		kMaxVendorStringLength = 64,
		kMaxProductStringLength = 64,
		kMaxEffectNameLength = 32,
	};

	struct EditorRect
	{
		Int16 top, left, bottom, right;
	};

	struct Event
	{
		Int32 type;
		Int32 byte_size;
		Int32 delta_frames;
		Int32 flags;
		char data[16];
	};

	struct Events
	{
		Int32 num_events;
		UIntNative reserved;
		Event * events[2];
	};

	struct MidiEvent
	{
		enum Flags
		{
			kFlagIsRealtime = 1 << 0	// live event (higher priority)
		};

		Int32 type = 1;
		Int32 bytesize;
		Int32 delta_frames;
		Int32 flags;
		Int32 note_length;
		Int32 note_offset;
		UInt32 midi_data;			// 1-3 MIDI bytes, byte 3 reserved
		char detune;				// -64 to +63 cents
		char note_off_velocity;		// [0, 127]
		char reserved1;
		char reserved2;
	};

	struct TimeInfo
	{
		enum Flags
		{
			kFlagTransportChanged = 1,
			kFlagTransportPlaying = 1 << 1,
			kFlagTransportCycleActive = 1 << 2,
			kFlagTransportRecording = 1 << 3,
			kFlagAutomationWriting = 1 << 6,
			kFlagAutomationReading = 1 << 7,
			kFlagNanosecondsValid = 1 << 8,
			kFlagPpqPositionValid = 1 << 9,
			kFlagTempoValid = 1 << 10,
			kFlagBarPositionValid = 1 << 11,
			kFlagCyclePositionValid = 1 << 12,
			kFlagTimeSignatureValid = 1 << 13,
			kFlagSmpteValid = 1 << 14,
			kFlagClockValid = 1 << 15
		};

		Float64 sample_pos;
		Float64 sample_rate;
		Float64 nano_seconds;			// nanoseconds
		Float64 ppq_pos;					// quarter note pos
		Float64 tempo;					// BPM
		Float64 bar_start_pos;
		Float64 cycle_start_pos;
		Float64 cycle_end_pos;
		Int32 time_signature_numerator;
		Int32 time_signature_denominator;
		Int32 smpte_offset;
		Int32 smpte_frame_rate;
		Int32 samples_to_next_clock;
		Int32 flags;
	};

	struct ParameterProperties
	{
		enum Flags
		{
			kFlagIsSwitch = 1 << 0,
			kFlagUsesIntegerMinMax = 1 << 1,
			kFlagUsesFloatStep = 1 << 2,
			kFlagUsesIntStep = 1 << 3,
			kFlagSupportsDisplayIndex = 1 << 4,
			kFlagSupportsDisplayCategory = 1 << 5,
			kFlagCanRamp = 1 << 6
		};

		Float32 step_float, small_step_float, large_step_float;
		char label[64];
		Int32 flags;
		Int32 min_integer;
		Int32 max_integer;
		Int32 step_integer;
		Int32 large_step_integer;
		char short_label[8];		// recommended: 6 + delimiter

		Int16 display_index;

		Int16 category;			// 0: no category, else index + 1
		Int16 num_parameters_in_category;
		Int16 reserved_1;
		char category_label[24];	// e.g. "Osc 1"

		char reserved_2[16];
	};

	struct PinProperties
	{
		enum Flags
		{
			kPinIsActive = 1 << 0,
			kPinIsStereo = 1 << 1,
			kPinUseSpeaker = 1 << 2
		};

		char label[64];
		Int32 flags;
		Int32 arrangement_type;
		char short_label[8];		// recommended: 6 + delimiter

		char reserved[48];
	};

	enum PluginCategory
	{
		kPluginCategoryUnknown = 0,
		kPluginCategoryEffect,
		kPluginCategorySynth,
		kPluginCategoryShell = 10,
	};

	enum SpeakerArrangement
	{
		kSpeakerArrangementUserDefined = -2,
		kSpeakerArrangementEmpty = -1,
		kSpeakerArrangementMono = 0,
		kSpeakerArrangementStereo,
		kSpeakerArrangementStereoSurround,
		kSpeakerArrangementStereoCenter,
		kSpeakerArrangementStereoSide,
		kSpeakerArrangementStereoCLfe,
		kSpeakerArrangement30Cine,
		kSpeakerArrangement30Music,
		kSpeakerArrangement31Cine,
		kSpeakerArrangement31Music,
		kSpeakerArrangement40Cine,
		kSpeakerArrangement40Music,
		kSpeakerArrangement41Cine,
		kSpeakerArrangement41Music,
		kSpeakerArrangement50,
		kSpeakerArrangement51,
		kSpeakerArrangement60Cine,
		kSpeakerArrangement60Music,
		kSpeakerArrangement61Cine,
		kSpeakerArrangement61Music,
		kSpeakerArrangement70Cine,
		kSpeakerArrangement70Music,
		kSpeakerArrangement71Cine,
		kSpeakerArrangement71Music,
		kSpeakerArrangement80Cine,
		kSpeakerArrangement80Music,
		kSpeakerArrangement81Cine,
		kSpeakerArrangement81Music,
		kSpeakerArrangement102,
	};

	constexpr CString::View kCanDoSendEvents = "sendVstEvents";					//send events to Host
	constexpr CString::View kCanDoSendMidiEvent = "sendVstMidiEvent";			//send MIDI events to host
	constexpr CString::View kCanDoReceiveEvents = "receiveVstEvents";			//receive MIDI events from host
	constexpr CString::View kCanDoReceiveMidiEvent = "receiveVstMidiEvent";
	constexpr CString::View kCanDoReceiveTimeInfo = "receiveVstTimeInfo";		//receive TimeInfo from Host

}

#if (defined(REFLEX_OS_WINDOWS) && !REFLEX_64BIT)
#pragma pack(pop)
#elif defined(__GNUC__)
#pragma pack(pop)
#endif
