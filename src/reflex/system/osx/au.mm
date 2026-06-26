#include "globals.h"
#include "window.h"

#include "../common/instance/plugin.h"
#include "../common/instance/eventqueue.h"

#include "../common/instance/instance.cpp"
#include "../common/instance/plugin.cpp"

#import <objc/runtime.h>




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

template <class CHARACTER> inline String <CHARACTER> Strip(String <CHARACTER> string, bool(*_Nonnull fn)(CHARACTER) _Nonnull)
{
	REFLEX_RLOOP(idx, string.GetSize())
	{
		if (fn(string[idx])) string.Remove(idx);
	}

	return std::move(string);
}

typedef ::AudioUnit _Nonnull AudioUnitRef;

const CFStringRef _Nonnull kDefaultParameterName = CFSTR("{No Parameter}");
const CFStringRef _Nonnull kVersionString = CFSTR("version");
const CFStringRef _Nonnull kTypeString = CFSTR("type");
const CFStringRef _Nonnull kSubtypeString = CFSTR("subtype");
const CFStringRef _Nonnull kManufacturerString = CFSTR("manufacturer");
const CFStringRef _Nonnull kNameString = CFSTR("name");
const CFStringRef _Nonnull kDataString = CFSTR("data");

const CFStringRef _Nonnull kDefaultPresetName = CFSTR("Default");

struct AudioComponentPlugInInstance
{
	AudioComponentPlugInInterface plugininterface;
	void * _Nullable a;	//_Nonnull (* _Nonnull mConstruct)(void * _Nonnull memory, AudioComponentInstance _Nonnull ci);
	void * _Nullable b;	//_Nonnull mDestruct)(void * _Nonnull memory);
	void * _Nullable pad[2];                // pad to a 16-byte boundary (in either 32 or 64 bit mode)
	UInt32 storage;       // the ACI implementation object is constructed into this memory
};

struct AudioUnitSession : public Object
{
	AudioUnitSession();

	~AudioUnitSession();

	#if !__has_feature(objc_arc)
	NSAutoreleasePool * _Nonnull m_autoreleasepool;
	#endif

	Common::PluginSession m_session;

	const Class _Nonnull m_viewfactoryclass;

	CFStringRef _Nonnull m_viewfactoryclassname;

	CFURLRef _Nonnull m_bundleref;



private:

	Class _Nonnull CreateViewFactoryClass(const AudioPlugin::Configuration & config);

	#if !__has_feature(objc_arc)
	static NSAutoreleasePool * _Nonnull Init();
	#endif

};

typedef The <AudioUnitSession> TheAudioUnitSession;

struct AudioUnitImpl : public Common::PluginInstance
{
	typedef AudioComponentPlugInInstance ACPI;


	AudioUnitImpl(Common::PluginSession & session, const Configuration::Class & cls, ::AudioUnit _Nonnull auComponentInstance);

	~AudioUnitImpl();


	void OnSetViewSize(const iSize & size) override {}

	void ReportProcessingDelay(UInt32 delay) override;

	void ReportStateChange(UInt8 change_flags) override;

	void BeginAutomation(UInt32 idx) override;

	void Automate(UInt32 idx, Float32 value) override;

	void EndAutomation(UInt32 idx) override;


	static AudioComponentMethod _Nullable AUEntryLookup(SInt16 selector);

	inline static void LogProperty(const char * _Nonnull method, AudioUnitPropertyID property)
	{
		Log(Join(method, " ", ToCString(property), " ", kAudioUnitPropertyNames[Min<Int>(property, 63)]).GetData());
	}

	static void AddNumToDictionary(CFMutableDictionaryRef _Nonnull dict, CFStringRef _Nonnull key, SInt32 value);

	bool PropertyChanged(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element);

	template <class TYPE> static void SetStandardSize(Reflex::UInt32 & size) {size = sizeof(TYPE);}

	inline static void SetWriteable(Boolean * _Nullable outWritable, bool value) { if (outWritable) *outWritable = value; }

	bool GetPropertyInfo(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, bool & writable, UInt32 & size);

	OSStatus GetProperty(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, void * _Nullable pdata, UInt32 &datasize);

	OSStatus SetProperty(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, const void * _Nonnull pdata, UInt32 datasize);


	void UpdateBufferSize(UInt32 buffersize);

	void UpdateSampleRate(Float64 newSampleRate);

	void UpdateParameter(UInt32 idx, Float32 value, UInt32 bufferpos = 0);	//host to plugin

	Float32 GetParameterImpl(UInt32 idx);


	static OSStatus BeatAndTime_Null(void * _Nonnull data, Float64 * _Nonnull ,Float64 * _Nonnull);

	static OSStatus TransportState_Null(void * _Nonnull data, Boolean * _Nonnull outIsPlaying, Boolean * _Nonnull outTransportStateChanged, Float64 * _Nonnull outCurrentSampleInTimeLine, Boolean * _Nonnull outIsCycling, Float64 * _Nonnull outCycleStartBeat, Float64 * _Nonnull outCycleEndBeat);

	static OSStatus MusicalTimeLocation_Null(void * _Nonnull hostdata, UInt32 * _Nonnull outDeltaSampleOffsetToNextBeat, Float32 * _Nonnull outTimeSig_Numerator, UInt32 * _Nonnull outTimeSig_Denominator, Float64 * _Nonnull outCurrentMeasureDownBeat);

	static OSStatus AudioUnitRender_NullInput(void * _Nonnull client, UInt32 * _Nonnull flags, const AudioTimeStamp * _Nonnull timestamp, UInt32 bus, UInt32 samples, AudioBufferList * _Nonnull buffers);

	static OSStatus Render(AudioUnitImpl & self, AudioUnitRenderActionFlags * _Nonnull flags, const AudioTimeStamp * _Nonnull timestamp, UInt32 bus, UInt32 samples, AudioBufferList * _Nonnull buffers);

	using PluginInstance::QueueEvent;


	AudioUnitRef m_audiounit;


	UInt32 m_delay;

	Array <AudioUnitParameterID> m_paramids;

	struct Parameter : public AudioUnitParameterInfo
	{
		Key32 namehash;
		Float32 value_cache;
	};

	Array <Parameter> m_params;



	//calbacks

	struct AU_HostCallbackInfo	//use this instead of offical one for compatibulity with ableton live
	{
		void * _Nonnull hostUserData;
		HostCallback_GetBeatAndTempo _Nonnull beatAndTempoProc;
		HostCallback_GetMusicalTimeLocation _Nonnull musicalTimeLocationProc;
		HostCallback_GetTransportState _Nonnull transportStateProc;
	};

	AU_HostCallbackInfo m_hostcallbackinfo;

	UInt64 m_buffer;	//to allow for expadnded size of 40;

	struct PropertyListener
	{
		AudioUnitPropertyID property;
		AudioUnitPropertyListenerProc _Nonnull callback;
		void * _Nonnull client;
	};

	Sequence <UInt32,PropertyListener> m_propertylisteners;

	AURenderCallbackStruct m_inputcallback;

	bool m_renderinplace;

	AudioBufferList m_inputbufferlist;

	AudioBuffer m_pad[7];



	//buffer

	Float64 m_rendertimestamp;

	AudioStreamBasicDescription m_streamformat;


	Array <AUChannelInfo> m_channelinfos;

	Array < Array <Float32> > m_buffers;


	#if (REFLEX_DEBUG)
	static const char * _Nonnull kAudioUnitPropertyNames[64];
	#else
	static const char * _Nonnull kAudioUnitPropertyNames[1];
	#endif

	static constexpr AudioUnitScope kScopes[2] = {kAudioUnitScope_Global, kAudioUnitScope_Part};

};

REFLEX_END_INTERNAL

@interface NS_PluginView : NSView
{
@public

	Reflex::System::OSX::AudioUnitImpl * m_instance;
}

- (id _Nonnull) init:(Reflex::System::OSX::AudioUnitImpl * _Nonnull)instance;
- (void) dealloc;
- (BOOL) isOpaque;
- (BOOL) isFlipped;
- (BOOL) acceptsFirstResponder;
- (BOOL) acceptsFirstMouse:(NSEvent * _Nullable)event;
- (void) rightMouseDown:(NSEvent * _Nonnull)event;
- (void) viewWillMoveToSuperview:(NSView * _Nullable) superview;

@end

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

AudioUnitSession::AudioUnitSession() :
	#if !__has_feature(objc_arc)
	m_autoreleasepool(Init()),
	#endif
	m_session((void*)0, Common::kPluginFormatAUv2),
	m_viewfactoryclass(CreateViewFactoryClass(m_session.config)),
	m_viewfactoryclassname(CFStringCreateWithCString(kCFAllocatorDefault, class_getName(m_viewfactoryclass), kCFStringEncodingUTF8))
{
	objc_registerClassPair(m_viewfactoryclass);

	CFRetain(kDefaultPresetName);
	
	auto bundle = MakeObjCRef([NSBundle bundleForClass: m_viewfactoryclass]);

	m_bundleref = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (__bridge CFStringRef)[bundle bundlePath], kCFURLPOSIXPathStyle, true);
}

AudioUnitSession::~AudioUnitSession()
{
	CFRelease(m_bundleref);

	CFRelease(kDefaultPresetName);

	CFRelease(m_viewfactoryclassname);

	objc_disposeClassPair(m_viewfactoryclass);
	
	//[m_autoreleasepool release];
}

#if !__has_feature(objc_arc)
NSAutoreleasePool * _Nonnull AudioUnitSession::Init()
{
	auto autoreleasepool = [[NSAutoreleasePool alloc] init];

	//[NSApplication sharedApplication];

	return autoreleasepool;
}
#endif

AudioUnitImpl::AudioUnitImpl(Common::PluginSession & session, const Configuration::Class & cls, AudioUnitRef audiounit)
	: PluginInstance(session, cls)
	, m_audiounit(audiounit)
	, m_delay(0)
	, m_renderinplace(false)
	, m_rendertimestamp(-1.0)
{
	MemClear(&m_hostcallbackinfo, sizeof(AU_HostCallbackInfo));

	m_hostcallbackinfo.beatAndTempoProc = &AudioUnitImpl::BeatAndTime_Null;
	m_hostcallbackinfo.transportStateProc = &AudioUnitImpl::TransportState_Null;
	//m_hostcallbackinfo.musicalTimeLocationProc = &AU_Instance::MusicalTimeLocation_Null;

	m_inputcallback.inputProc = reinterpret_cast<AURenderCallback>(&AudioUnitImpl::AudioUnitRender_NullInput);
	m_inputcallback.inputProcRefCon = 0;

	m_inputbufferlist.mNumberBuffers = 0;
	m_inputbufferlist.mBuffers[0].mData = 0;

	m_streamformat.mSampleRate = 44100.0;
	m_streamformat.mFormatID = kAudioFormatLinearPCM;
	m_streamformat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
	m_streamformat.mBytesPerPacket = 4;
	m_streamformat.mFramesPerPacket = 1;
	m_streamformat.mBytesPerFrame = 4;
	m_streamformat.mChannelsPerFrame = 2;	//WAS ONE
	m_streamformat.mBitsPerChannel = 32;
	m_streamformat.mReserved = 0;



	//cover case where params where dropped from versions

	m_paramids.Allocate(64);

	m_params.Allocate(64);



	//create

	m_paramids.SetSize(cls.num_params);

	m_params.SetSize(cls.num_params);

	REFLEX_LOOP(idx, cls.num_params)
	{
		m_paramids[idx] = idx;

		Parameter & param = m_params[idx];

		param.name[0] = 0;
		param.unit = kAudioUnitParameterUnit_Generic;
		param.unitName = 0;
		param.clumpID = 0;
		param.cfNameString = 0;
		param.minValue = 0.0f;
		param.maxValue = 1.0f;
		param.defaultValue = 0.0f;
		//kAudioUnitParameterFlag_IsGlobalMeta is to work around apparent au validator issue
		param.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsGlobalMeta;
		param.value_cache = 0.0f;

		//CFRetain(param.cfNameString);
	}


	m_buffers.SetSize(Max(cls.channels_io.a, cls.channels_io.b));

	m_channelinfos.SetSize(m_buffers.GetSize() / 2);

	MemClear(m_channelinfos.GetData(), sizeof(AUChannelInfo) * m_channelinfos.GetSize());

	REFLEX_LOOP(idx, cls.channels_io.a / 2) m_channelinfos[idx].inChannels = 2;

	REFLEX_LOOP(idx, cls.channels_io.b / 2) m_channelinfos[idx].outChannels = 2;


	Initialise();

	UpdateBufferSize(1024);

	UpdateSampleRate(m_streamformat.mSampleRate);


	ReportStateChange(kChangeParameterInfo | kChangeParameterValues);	//initialise params
}

AudioUnitImpl::~AudioUnitImpl()
{
	DiscardEditor();

	Deinitialise();
}

void AudioUnitImpl::ReportStateChange(UInt8 change_flags)
{
	REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::ReportStateChange");

	if (change_flags & (kChangeParameterValues | kChangeParameterInfo))
	{
		auto & client = GetCallbacks();

		CString name;

		bool info = True(change_flags & kChangeParameterInfo);

		REFLEX_LOOP(idx, m_params.GetSize())
		{
			auto & param = m_params[idx];

			if (info)
			{
				client.OnGetParameterInfo(idx, name);

				RawStringCopy(name.GetData(), param.name, sizeof(param.name));
			}

			param.value_cache = client.OnGetParameterValue(idx);
		}

		PropertyChanged(kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0);
	}
}

void AudioUnitImpl::ReportProcessingDelay(UInt32 delay)
{
	REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::ReportProcessingDelay");

	m_delay = delay;

	PropertyChanged(kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0);
}

void AudioUnitImpl::BeginAutomation(UInt32 idx)
{
	AudioUnitEvent auevent;

	auevent.mEventType = kAudioUnitEvent_BeginParameterChangeGesture;

	auevent.mArgument.mParameter.mAudioUnit = m_audiounit;

	auevent.mArgument.mParameter.mParameterID = idx;

	auevent.mArgument.mParameter.mScope = kAudioUnitScope_Global;

	auevent.mArgument.mParameter.mElement = 0;

	AUEventListenerNotify(NULL, NULL, &auevent);
}

void AudioUnitImpl::Automate(UInt32 idx, Float32 value)
{
	m_params[idx].value_cache = value;

	AudioUnitEvent auevent;

	auevent.mEventType = kAudioUnitEvent_ParameterValueChange;

	auevent.mArgument.mParameter.mAudioUnit = m_audiounit;

	auevent.mArgument.mParameter.mParameterID = idx;

	auevent.mArgument.mParameter.mScope = kAudioUnitScope_Global;

	auevent.mArgument.mParameter.mElement = 0;

	AUEventListenerNotify(NULL, NULL, &auevent);
}

void AudioUnitImpl::EndAutomation(UInt32 idx)
{
	AudioUnitEvent auevent;

	auevent.mEventType = kAudioUnitEvent_EndParameterChangeGesture;

	auevent.mArgument.mParameter.mAudioUnit = m_audiounit;

	auevent.mArgument.mParameter.mParameterID = idx;

	auevent.mArgument.mParameter.mScope = kAudioUnitScope_Global;

	auevent.mArgument.mParameter.mElement = 0;

	AUEventListenerNotify(NULL, NULL, &auevent);
}

bool AudioUnitImpl::GetPropertyInfo(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, bool & writable, UInt32 & size)
{
	writable = false;

	size = 0;

	switch (property)
	{
		case kAudioUnitProperty_ClassInfo:
			writable = true;
			SetStandardSize<CFPropertyListRef>(size);
			return true;

		case kAudioUnitProperty_CurrentPreset:	//for DP
		case kAudioUnitProperty_PresentPreset:
			writable = true;
			SetStandardSize<AUPreset>(size);
			return true;

		case kAudioUnitProperty_MakeConnection:
			writable = true;
			SetStandardSize<AudioUnitConnection>(size);
			return true;

//		case kAudioUnitProperty_FastDispatch:
//			if (element == kAudioUnitRenderSelect || element == kMusicDeviceMIDIEventSelect)
//			{
//				readable = true;
//				SetStandardSize<void*>(size);
//			}
//			break;

		case kAudioUnitProperty_SetRenderCallback:
			writable = true;
			SetStandardSize<AURenderCallbackStruct>(size);
			return true;

		case kAudioUnitProperty_InPlaceProcessing:
			writable = true;
			SetStandardSize<UInt32>(size);
			return true;

		case kAudioUnitProperty_SampleRate:
			writable = true;
			SetStandardSize<Float64>(size);
			return true;

		case kAudioUnitProperty_StreamFormat:
			writable = true;
			SetStandardSize<AudioStreamBasicDescription>(size);
			return true;

		case kAudioUnitProperty_ParameterList:
			if (scope == kAudioUnitScope_Global) size = m_paramids.GetSize() * sizeof(AudioUnitParameterID);
			return true;

		case kAudioUnitProperty_ParameterInfo:
			SetStandardSize<AudioUnitParameterInfo>(size);
			return true;

		case kAudioUnitProperty_ElementCount:
			SetStandardSize<UInt32>(size);
			return true;

		case kAudioUnitProperty_Latency:
			if (scope == kAudioUnitScope_Global) SetStandardSize<Float64>(size);
			return true;

		case kAudioUnitProperty_SupportedNumChannels:
			// MIDI-only units (channels_io == {0,0}) have an empty m_channelinfos
			// and must not expose this property at all — auval reads an empty
			// AUChannelInfo array as "no supported channel configurations" and
			// fails "Default Format of unit does not match reported Channel
			// handling capabilities" on a unit that legitimately has zero audio
			// buses. Returning false here makes GetProperty report
			// kAudioUnitErr_InvalidProperty via the existing guard.
			if (m_channelinfos.GetSize() == 0) return false;
			size = m_channelinfos.GetSize() * sizeof(AUChannelInfo);
			return true;

		case kAudioUnitProperty_MaximumFramesPerSlice:
			if (scope == kAudioUnitScope_Global)
			{
				writable = true;
				SetStandardSize<UInt32>(size);
			}
			return true;

		case kAudioUnitProperty_HostCallbacks:
			writable = true;
			SetStandardSize<AU_HostCallbackInfo>(size);
			return true;

		case kAudioUnitProperty_ElementName:
			SetStandardSize<CFStringRef>(size);
			return true;

		case kAudioUnitProperty_LastRenderError:
			if (scope == kAudioUnitScope_Global) SetStandardSize<ComponentResult>(size);
			return true;

		case kMusicDeviceProperty_InstrumentCount:
			if (scope == kAudioUnitScope_Global) SetStandardSize<UInt32>(size);
			return true;

//		case kAudioUnitProperty_BypassEffect:
//			readable = true;
//			SetStandardSize<UInt32>(size);
//			break;

		case kAudioUnitProperty_CocoaUI:
			if (scope == kAudioUnitScope_Global) SetStandardSize<AudioUnitCocoaViewInfo>(size);
			return true;

		case 64000:
			SetStandardSize<__unsafe_unretained NS_PluginView**>(size);
			return true;

		default:
			//Trace("GetPropertyInfo NOT SUPPORTED", kAudioUnitPropertyNames[Min<UInt>(property,GetArraySize(kAudioUnitPropertyNames) - 1)]);
			return false;
	}

	return true;
}

ComponentResult AudioUnitImpl::GetProperty(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, void * _Nullable data, UInt32 & sizeio)
{
	bool writable = false;

	UInt32 requiredsize = 0;

	if (!GetPropertyInfo(property, scope, element, writable, requiredsize)) return kAudioUnitErr_InvalidProperty;

	if (!data || sizeio < requiredsize) return kAudioUnitErr_InvalidProperty;	//some hosts skip GetPropertyInfo

	sizeio = Min(sizeio, requiredsize);

	switch (property)
	{
		case kAudioUnitProperty_ClassInfo:
			{
				auto chunk = GetCallbacks().OnGetPluginChunk();

				CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

				AddNumToDictionary(dict, kVersionString, 1);

				AddNumToDictionary(dict, kTypeString, Configuration::Class::AudioUnit::kTypes[cls.type]);
				AddNumToDictionary(dict, kManufacturerString, cls.audiounit.company_4cc);
				AddNumToDictionary(dict, kSubtypeString, cls.audiounit.uid_4cc);

				CFDictionarySetValue(dict, kNameString, kDefaultPresetName);

				CFDataRef cfdataref = CFDataCreate(kCFAllocatorDefault, chunk.GetData(), chunk.GetSize());

				CFDictionarySetValue(dict, kDataString, cfdataref);

				CFRelease(cfdataref);

				*reinterpret_cast<CFPropertyListRef*>(data) = dict;
			}
			return noErr;

		case kAudioUnitProperty_CurrentPreset:	//DP
		case kAudioUnitProperty_PresentPreset:
			if (Search(kScopes, scope))
			{
				auto & aupreset = *reinterpret_cast<AUPreset*>(data);
				aupreset.presetNumber = 0;
				aupreset.presetName = kDefaultPresetName;
				return noErr;
			}
			else
			{
				return kAudioUnitErr_InvalidProperty;
			}

//		case kAudioUnitProperty_FastDispatch:
//			switch (element)
//			{
//				case kAudioUnitRenderSelect:
//					*(AudioUnitRenderProc*)data = reinterpret_cast<AudioUnitRenderProc>(&AudioUnit::AudioUnitRender_Fast);
//					break;
//
//				case kMusicDeviceMIDIEventSelect:
//					*(MusicDeviceMIDIEventProc*)data = reinterpret_cast<MusicDeviceMIDIEventProc>(&AudioUnit::MusicDeviceMIDIEvent_Fast);
//					break;
//
//				default:
//					*(MusicDeviceMIDIEventProc*)data = 0;
//					break;
//			}
//			break;

		case kAudioUnitProperty_InPlaceProcessing:
			(*Reinterpret<UInt32>(data)) = m_renderinplace;
			return noErr;

		case kAudioUnitProperty_SampleRate:
			(*Reinterpret<Float64>(data)) = m_streamformat.mSampleRate;
			return noErr;

		case kAudioUnitProperty_StreamFormat:
		{
			UInt32 nchannels[] = {0, GetAudioChannels(false).size, GetAudioChannels(true).size, 0, 0, 0};

			m_streamformat.mChannelsPerFrame = nchannels[scope] ? 2 : 0;

			(*Reinterpret<AudioStreamBasicDescription>(data)) = m_streamformat;

			if (!m_streamformat.mChannelsPerFrame) return kAudioUnitErr_InvalidProperty;;

			return noErr;
		}

		case kAudioUnitProperty_ElementCount:
			{
				UInt32 nchannels[] = {0, GetAudioChannels(false).size, GetAudioChannels(true).size, 0, 0, 0};

				(*Reinterpret<UInt32>(data)) = nchannels[scope] / 2;
			}
			return noErr;

		case kAudioUnitProperty_SupportedNumChannels:
			if (scope == kAudioUnitScope_Global)
			{
				auto channelinfos = Reinterpret<AUChannelInfo>(data);

				REFLEX_LOOP(idx, m_channelinfos.GetSize())
				{
					channelinfos[idx] = m_channelinfos[idx];
				}

				return noErr;
			}
			else
			{
				return kAudioUnitErr_InvalidProperty;
			}

		case kAudioUnitProperty_ParameterList:
			if (scope == kAudioUnitScope_Global)
			{
				MemCopy(m_paramids.GetData(), data, m_paramids.GetSize() * sizeof(AudioUnitParameterID));
				return noErr;
			}
			else
			{
				return kAudioUnitErr_InvalidProperty;
			}

		case kAudioUnitProperty_ParameterInfo:
			(*Reinterpret<AudioUnitParameterInfo>(data)) = m_params[element];
			return noErr;

		case kAudioUnitProperty_ElementName:
		{
			const char * labels[] = {"", "Input", "Output", "", "", ""};
			UInt idx = (element * 2) + 1;
			auto cstring = Join(labels[scope], " ", ToCString(idx), "+", ToCString(idx + 1));
			*reinterpret_cast<CFStringRef*>(data) = CFStringCreateWithCString(0, cstring.GetData(), kCFStringEncodingMacRoman);
			return noErr;
		}

		case kAudioUnitProperty_Latency:
			if (scope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidProperty;
			(*Reinterpret<Float64>(data)) = Float64(m_delay) / m_streamformat.mSampleRate;
			return noErr;

		case kAudioUnitProperty_MaximumFramesPerSlice:
			if (scope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidProperty;
			*Reinterpret<UInt32>(data) = GetCurrentBufferSize();
			return noErr;

		case kAudioUnitProperty_LastRenderError:
			if (scope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidProperty;
			*Reinterpret<ComponentResult>(data) = noErr;
			return noErr;

//		case kAudioUnitProperty_BypassEffect:
//			*reinterpret_cast<UInt32*>(data) = 0;
//			break;

		case kMusicDeviceProperty_InstrumentCount:
			*Reinterpret<UInt32>(data) = 0;
			return noErr;

		case kAudioUnitProperty_CocoaUI:
		{
			auto & info = *Cast<AudioUnitCocoaViewInfo>(data);

			auto session = TheAudioUnitSession::Get();
			
			CFRetain(session->m_bundleref);
			
			CFRetain(session->m_viewfactoryclassname);

			info = { session->m_bundleref, session->m_viewfactoryclassname };
			
			return noErr;
		}

		case 64000:	//custom
		{
			NSView * nspluginview = [[NS_PluginView alloc]init:this];
			ShowEditor((__bridge void*)nspluginview);
			*reinterpret_cast<__unsafe_unretained NSView**>(data) = nspluginview;
			return noErr;
		}

		default:
			return kAudioUnitErr_InvalidProperty;
	}
}

ComponentResult AudioUnitImpl::SetProperty(AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, const void * _Nonnull pdata, UInt32 size)
{
	bool writable = false;

	UInt32 requiredsize = 0;

	GetPropertyInfo(property, scope, element, writable, requiredsize);

	if (!writable || size < requiredsize) return kAudioUnitErr_InvalidProperty;

	switch (property)
	{
		case kAudioUnitProperty_ClassInfo:
		{
			auto dict = *Reinterpret<CFDictionaryRef>(pdata);

			CFDataRef cfdataref = static_cast<CFDataRef>(CFDictionaryGetValue(dict, kDataString));

			Array<UInt8>::View pluginchunk(CFDataGetBytePtr(cfdataref), UInt32(CFDataGetLength(cfdataref)));

			GetCallbacks().OnSetPluginChunk(pluginchunk);

			PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0);
			ReportStateChange(kChangeParameterInfo | kChangeParameterValues);
			return noErr;
		}

		case kAudioUnitProperty_CurrentPreset:	//DP
		case kAudioUnitProperty_PresentPreset:
			return noErr;

		case kAudioUnitProperty_MakeConnection:	//for AU VAL
			return noErr;

// 		{
// 			Trace("set kAudioUnitProperty_MakeConnection", element);
// 			const AudioUnitConnection & connection = *reinterpret_cast<const AudioUnitConnection*>(pdata);
//
// 			if (connection.sourceAudioUnit != 0)
// 			{
// 				PropertyChanged(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0);
// 				PropertyChanged(kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0);
// 			}
//
// 			bool changedConnection = false;
// 			bool changedRenderCallback = false;
//
// 			//m_inputconnection = connection;
//
// 			PropertyChanged(kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, element);
//
// 			rendercallback.inputProc = 0;
// 			break;
// 		}

		case kAudioUnitProperty_SetRenderCallback:
		{
			auto & rendercallback = *Reinterpret<AURenderCallbackStruct>(pdata);

			if (rendercallback.inputProc)
			{
				m_inputcallback = rendercallback;
			}
			else
			{
				m_inputcallback.inputProc = reinterpret_cast<AURenderCallback>(&AudioUnitImpl::AudioUnitRender_NullInput);
			}

			PropertyChanged(kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, element);
			return noErr;
		}

		case kAudioUnitProperty_InPlaceProcessing:
			m_renderinplace = True(*Reinterpret<UInt32>(pdata));
			return noErr;

		case kAudioUnitProperty_StreamFormat:
			if (Reinterpret<AudioStreamBasicDescription>(pdata)->mChannelsPerFrame != 2) return kAudioUnitErr_InvalidProperty;
			UpdateSampleRate(Reinterpret<AudioStreamBasicDescription>(pdata)->mSampleRate);
			return noErr;

		case kAudioUnitProperty_SampleRate:
			UpdateSampleRate(*Reinterpret<Float64>(pdata));
			return noErr;

		case kAudioUnitProperty_MaximumFramesPerSlice:
			UpdateBufferSize(*Reinterpret<SInt32>(pdata));
			return noErr;

		case kAudioUnitProperty_HostCallbacks:
			m_hostcallbackinfo = *Reinterpret<AU_HostCallbackInfo>(pdata);
			if (!m_hostcallbackinfo.beatAndTempoProc) m_hostcallbackinfo.beatAndTempoProc = &AudioUnitImpl::BeatAndTime_Null;
			if (!m_hostcallbackinfo.transportStateProc) m_hostcallbackinfo.transportStateProc = &AudioUnitImpl::TransportState_Null;
			//if (!m_hostcallbackinfo.musicalTimeLocationProc) m_hostcallbackinfo.musicalTimeLocationProc = &AU_Instance::MusicalTimeLocation_Null;
			PropertyChanged(kAudioUnitProperty_HostCallbacks, kAudioUnitScope_Global, 0);
			return noErr;

		default:
			//Trace("SetProperty NOT SUPPORTED", kAudioUnitPropertyNames[Min<UInt32>(property,GetArraySize(kAudioUnitPropertyNames) - 1)]);
			return kAudioUnitErr_InvalidProperty;
	}

	return noErr;
}

void AudioUnitImpl::AddNumToDictionary(CFMutableDictionaryRef _Nonnull dict, CFStringRef _Nonnull key, SInt32 value)
{
	CFNumberRef num = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);

	CFDictionarySetValue(dict, key, num);

	CFRelease(num);
}

void AudioUnitImpl::UpdateSampleRate(Float64 samplerate)
{
	if (PrepareSampleRate(Float32(samplerate)))
	{
		m_streamformat.mSampleRate = samplerate;

		if (GetAudioChannels(false))
		{
			PropertyChanged(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0);
			PropertyChanged(kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0);
		}

		if (GetAudioChannels(true))
		{
			PropertyChanged(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0);
			PropertyChanged(kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, 0);
		}
	}
}

void AudioUnitImpl::UpdateBufferSize(UInt32 buffersize)
{
	if (PrepareBufferSize(buffersize))
	{
		for (auto & i : m_buffers)
		{
			i.SetSize(buffersize);

			i.Wipe();
		}

		auto inputs = GetAudioChannels(false);

		auto outputs = GetAudioChannels(true);

		m_inputbufferlist.mNumberBuffers = inputs.size;

		REFLEX_LOOP(idx, inputs.size)
		{
			m_inputbufferlist.mBuffers[idx].mNumberChannels = 1;

			m_inputbufferlist.mBuffers[idx].mDataByteSize = buffersize * 4;

			m_inputbufferlist.mBuffers[idx].mData = m_buffers[idx].GetData();
		}

		REFLEX_LOOP(idx, outputs.size)
		{
			outputs[idx] = m_buffers[idx].GetData();
		}

		PropertyChanged(kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0);
	}
}

REFLEX_INLINE void AudioUnitImpl::UpdateParameter(UInt idx, Float32 value, UInt32 bufferpos)
{
	m_params[idx].value_cache = value;

	if (GetThreadID() == kMainThreadID)
	{
		GetCallbacks().OnSetParameterValue(idx, value);
	}
	else
	{
		QueueEvent(Event::kTypeAutomation, UInt16(bufferpos), idx, value);
	}
}

REFLEX_INLINE Float32 AudioUnitImpl::GetParameterImpl(UInt idx)
{
	return m_params[idx].value_cache;
}

OSStatus AudioUnitImpl::BeatAndTime_Null(void * _Nonnull data, Float64 * _Nonnull beatpos, Float64 * _Nonnull tempo)
{
	*beatpos = 0.0;
	*tempo = 120.0;

	return noErr;
}

OSStatus AudioUnitImpl::TransportState_Null(void * _Nonnull data, Boolean * _Nonnull outIsPlaying, Boolean * _Nonnull outTransportStateChanged, Float64 * _Nonnull outCurrentSampleInTimeLine, Boolean * _Nonnull outIsCycling, Float64 * _Nonnull outCycleStartBeat, Float64 * _Nonnull outCycleEndBeat)
{
	*outIsPlaying = false;

	return noErr;
}

OSStatus AudioUnitImpl::MusicalTimeLocation_Null(void * _Nonnull inHostUserData, UInt32 * _Nonnull outDeltaSampleOffsetToNextBeat, Float32 * _Nonnull outTimeSig_Numerator, UInt32 * _Nonnull outTimeSig_Denominator, Float64 * _Nonnull outCurrentMeasureDownBeat)
{
	return noErr;
}

OSStatus AudioUnitImpl::AudioUnitRender_NullInput(void * _Nonnull client, UInt32 * _Nonnull flags, const AudioTimeStamp * _Nonnull timestamp, UInt32 bus, UInt32 samples, AudioBufferList * _Nonnull buffers)
{
	return noErr;
}

REFLEX_INLINE OSStatus AudioUnitImpl::Render(AudioUnitImpl & self, AudioUnitRenderActionFlags * _Nonnull flags, const AudioTimeStamp * _Nonnull timestamp, UInt32 bus, UInt32 samples, AudioBufferList * _Nonnull buffers)
{
	if (samples > self.GetCurrentBufferSize()) return 1;	//for STUPID AUVAL

	OSStatus rtn = noErr;

	if (SetFiltered(self.m_rendertimestamp, timestamp->mSampleTime))
	{
		Boolean isplaying = false;
		Float64 beatpos = 0.0;
		Float64 tempo = 120.0;

		Boolean temp = false;
		Float64 temp64;
	
		auto & hostcallbacks = self.m_hostcallbackinfo;

		hostcallbacks.transportStateProc(hostcallbacks.hostUserData, &isplaying, &temp, &temp64, &temp, &temp64, &temp64);

		hostcallbacks.beatAndTempoProc(hostcallbacks.hostUserData, &beatpos, &tempo);

		if (auto inputs = self.GetAudioChannels(false))	//currently will only work if 2 inputs because assuming 1 x bus
		{
			auto & inputcallback = self.m_inputcallback;

			if (self.m_renderinplace)
			{
				REFLEX_LOOP(idx, inputs.size)
				{
					inputs[idx] = (Float32*)buffers->mBuffers[idx].mData;
				}

				AudioUnitRenderActionFlags flags = 0;

				rtn = inputcallback.inputProc(inputcallback.inputProcRefCon, &flags, timestamp, bus, samples, 0);
			}
			else
			{
				REFLEX_LOOP(idx, inputs.size)
				{
					Float32 * buffer = self.m_buffers[idx].GetData();

					inputs[idx] = buffer;

					self.m_inputbufferlist.mBuffers[idx].mDataByteSize = samples * 4;

					self.m_inputbufferlist.mBuffers[idx].mData = buffer;
				}

				AudioUnitRenderActionFlags flags = 0;

				rtn = self.m_inputcallback.inputProc(inputcallback.inputProcRefCon, &flags, timestamp, bus, samples, &self.m_inputbufferlist);

				REFLEX_LOOP(idx, inputs.size)
				{
					inputs[idx] = (Float32*)self.m_inputbufferlist.mBuffers[idx].mData;	//because reaper uses own buffers
				}
			}
		}

		self.ProcessRt(samples, isplaying, tempo * kRcpSixty, beatpos);
	}

	//also, this can be optimised to processing in place, if detect that iodata is not null

	REFLEX_LOOP(idx, 2)
	{
		AudioBuffer & buffer = buffers->mBuffers[idx];

		void * & channel = buffer.mData;

		if (channel)
		{
			MemCopy(self.m_buffers[(bus * 2) + idx].GetData(), channel, buffer.mDataByteSize);
		}
		else
		{
			channel = self.m_buffers[(bus * 2) + idx].GetData();
		}
	}

	return rtn;
}

bool AudioUnitImpl::PropertyChanged(AudioUnitPropertyID propertyid, AudioUnitScope scope, AudioUnitElement element)
{
	if (auto idx = m_propertylisteners.Search(propertyid))
	{
		auto & propertylistener = m_propertylisteners[idx.value].value;

		(propertylistener.callback)(propertylistener.client, m_audiounit, propertyid, scope, element);

		return true;
	}

	return false;
}

AudioComponentMethod _Nullable AudioUnitImpl::AUEntryLookup(SInt16 selector)
{
	REFLEX_USE(Reflex);

	typedef AudioComponentMethod Method;

	typedef void * _Null_unspecified ClientData;

	REFLEX_LOCAL(OSStatus,AUMethodInitialize)(void * client)
	{
		return noErr;
	}
	REFLEX_END;

	REFLEX_LOCAL(OSStatus,AUMethodGetPropertyInfo)(ACPI & acpi, AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement elem, UInt32 * _Nullable size, Boolean * _Nullable outWritable)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		bool writeable = false;

		UInt outsize = 0;

		if (!size) size = &outsize;

		auto result = self.GetPropertyInfo(property, scope, elem, writeable, *size) ? OSStatus(noErr) : kAudioUnitErr_InvalidProperty;

		if (outWritable) *outWritable = writeable;

		return result;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,AUMethodGetProperty)(ACPI & acpi, AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, void * pdata, UInt32 * size)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		UInt outsize = 0;

		if (!size) size = &outsize;

		if (!pdata) return kAudioUnitErr_InvalidProperty;

		return self.GetProperty(property, scope, element, pdata, *size);
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,AUMethodSetProperty)(ACPI & acpi, AudioUnitPropertyID property, AudioUnitScope scope, AudioUnitElement element, const void * _Nonnull pdata, UInt32 datasize)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		return self.SetProperty(property, scope, element, pdata, datasize);
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus, AddPropertyListener)(ACPI & acpi, AudioUnitPropertyID propertyid, AudioUnitPropertyListenerProc callback, ClientData client)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		if (!self.m_propertylisteners.Search(propertyid))
		{
			auto & listener = self.m_propertylisteners.Insert(propertyid);

			listener.property = propertyid;
			listener.callback = callback;
			listener.client = client;
		}

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,RemovePropertyListener)(ACPI & acpi, AudioUnitPropertyID propertyid, AudioUnitPropertyListenerProc callback)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		if (auto idx = self.m_propertylisteners.Search(propertyid))
		{
			self.m_propertylisteners.Remove(idx.value);
		}

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus, AddRemoveRenderNotify)(ACPI & acpi, AURenderCallback proc, ClientData client)
	{
		//TODO support this, seems mandatory
		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,AudioUnitGetParameter)(ACPI & acpi, AudioUnitParameterID parameterid, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue & value)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		value = self.GetParameterImpl(parameterid);

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,AudioUnitSetParameter)(ACPI & acpi, AudioUnitParameterID parameterid, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferpos)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		self.UpdateParameter(parameterid, value, bufferpos);

		return noErr;
	}
	REFLEX_END;

	REFLEX_LOCAL(OSStatus,AudioUnitScheduleParameters)(ACPI & acpi, const AudioUnitParameterEvent * events, UInt32 n)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		REFLEX_LOOP_PTR(events, event, n)
		{
			if (event->eventType == kParameterEvent_Immediate)
			{
				auto & immediate = event->eventValues.immediate;

				self.UpdateParameter(event->parameter, immediate.value, immediate.bufferOffset);
			}
		}

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,AudioUnitReset)(ACPI & acpi, AudioUnitScope scope, AudioUnitElement elem)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		self.m_rendertimestamp = -1.0;

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,AudioUnitRender)(ACPI & acpi, AudioUnitRenderActionFlags * _Nonnull flags, const AudioTimeStamp * _Nonnull timestamp, UInt32 bus, UInt32 samples, AudioBufferList * _Nonnull buffers)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		return Render(self, flags, timestamp, bus, samples, buffers);
	}
	REFLEX_END

	REFLEX_LOCAL(OSStatus,MusicDeviceMIDIEvent)(ACPI & acpi, UInt32 status, UInt32 data1, UInt32 data2, UInt32 sample)
	{
		auto & self = *Reinterpret<AudioUnitImpl>(&acpi.storage);

		UInt8 mididata[4] = {UInt8(status), UInt8(data1), UInt8(data2), 0};

		self.QueueEvent(Event::kTypeMIDI, UInt16(sample), 0, *reinterpret_cast<UInt32*>(mididata));

		return noErr;
	}
	REFLEX_END

	switch (selector)
	{
		case kAudioUnitInitializeSelect:
		case kAudioUnitUninitializeSelect:
			REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryLookup");
			return (AudioComponentMethod)AUMethodInitialize::Call;

		case kAudioUnitGetPropertyInfoSelect:
			REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryLookup");
			return (AudioComponentMethod)AUMethodGetPropertyInfo::Call;

		case kAudioUnitGetPropertySelect:
			REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryLookup");
			return (AudioComponentMethod)AUMethodGetProperty::Call;

		case kAudioUnitSetPropertySelect:
			REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryLookup");
			return (AudioComponentMethod)AUMethodSetProperty::Call;

		case kAudioUnitAddPropertyListenerSelect:
			REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryLookup");
			return (AudioComponentMethod)AddPropertyListener::Call;

		case kAudioUnitRemovePropertyListenerSelect:
		case kAudioUnitRemovePropertyListenerWithUserDataSelect:	//TEMP FIX
			REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryLookup");
			return (AudioComponentMethod)RemovePropertyListener::Call;

		case kAudioUnitAddRenderNotifySelect:
		case kAudioUnitRemoveRenderNotifySelect:
			return (AudioComponentMethod)AddRemoveRenderNotify::Call;


		case kAudioUnitGetParameterSelect:
			return (AudioComponentMethod)AudioUnitGetParameter::Call;

		case kAudioUnitSetParameterSelect:
			return (AudioComponentMethod)AudioUnitSetParameter::Call;

		case kAudioUnitScheduleParametersSelect:
			return (AudioComponentMethod)AudioUnitScheduleParameters::Call;


		case kAudioUnitRenderSelect:
			return (AudioComponentMethod)AudioUnitRender::Call;

		case kAudioUnitResetSelect:
			return (AudioComponentMethod)AudioUnitReset::Call;

		case kMusicDeviceMIDIEventSelect:
			return (AudioComponentMethod)MusicDeviceMIDIEvent::Call;

		default:
			break;
	};

	return 0;
}

Class _Nonnull AudioUnitSession::CreateViewFactoryClass(const AudioPlugin::Configuration & config)
{
	auto & cls = config.classes.GetFirst();

	auto clsname = Strip(Join(cls.vendor, cls.product, "AudioUnitViewFactory_"), &IsWhiteSpace);

	auto objc_cls = objc_allocateClassPair([NSObject class], clsname.GetData(), 0);
	
	FunctionPointer <unsigned(id,SEL)> interfaceVersion = [](id _Nullable self, SEL _Nullable _cmd) -> unsigned
	{
		return 0;
	};
	
	FunctionPointer <NSString*(id,SEL)> description = [](id _Nullable self, SEL _Nullable _cmd) -> NSString * _Nonnull
	{
		 return [@"Default View" copy];
	};
	
	FunctionPointer <NSView*(id,SEL,::AudioUnit,NSSize)> uiViewForAudioUnit = [](id _Nullable self, SEL _Nullable _cmd, ::AudioUnit _Nonnull audiounit, NSSize inPreferredSize)
	{
		NSView * nspluginview = 0;

		::UInt32 datasize = sizeof(void*);

		AudioUnitGetProperty(audiounit, 64000, 0, 0, &nspluginview, &datasize);

		[nspluginview autorelease];

		return nspluginview;
	};
	
	class_addMethod(objc_cls, @selector(interfaceVersion), (IMP)interfaceVersion, "I@:");
	class_addMethod(objc_cls, @selector(description), (IMP)description, "@@:");
	class_addMethod(objc_cls, @selector(uiViewForAudioUnit:withSize:), (IMP)uiViewForAudioUnit, "@@:^^{ComponentInstanceRecord}{CGSize=dd}");

	class_addProtocol(objc_cls, @protocol(AUCocoaUIBase));

	return objc_cls;
}

#if (REFLEX_DEBUG)
const char * _Nonnull AudioUnitImpl::kAudioUnitPropertyNames[] =
{
	"kAudioUnitProperty_ClassInfo",
	"kAudioUnitProperty_MakeConnection",
	"kAudioUnitProperty_SampleRate",
	"kAudioUnitProperty_ParameterList",
	"kAudioUnitProperty_ParameterInfo",
	"kAudioUnitProperty_FastDispatch",
	"kAudioUnitProperty_CPULoad",
	"{unknown}",
	"kAudioUnitProperty_StreamFormat",
	"{unknown}",
	"{unknown}",
	"kAudioUnitProperty_ElementCount",
	"kAudioUnitProperty_Latency",
	"kAudioUnitProperty_SupportedNumChannels",
	"kAudioUnitProperty_MaximumFramesPerSlice",
	"kAudioUnitProperty_SetExternalBuffer",
	"kAudioUnitProperty_ParameterValueStrings",
	"{unknown}",
	"kAudioUnitProperty_GetUIComponentList",
	"kAudioUnitProperty_AudioChannelLayout",
	"kAudioUnitProperty_TailTime",
	"kAudioUnitProperty_BypassEffect",
	"kAudioUnitProperty_LastRenderError",
	"kAudioUnitProperty_SetRenderCallback",
	"kAudioUnitProperty_FactoryPresets",
	"kAudioUnitProperty_ContextName",
	"kAudioUnitProperty_RenderQuality",
	"kAudioUnitProperty_HostCallbacks",
	"{unknown}",
	"kAudioUnitProperty_InPlaceProcessing",
	"kAudioUnitProperty_ElementName",
	"kAudioUnitProperty_CocoaUI",
	"kAudioUnitProperty_SupportedChannelLayoutTags",
	"kAudioUnitProperty_ParameterStringFromValue",
	"kAudioUnitProperty_ParameterIDName",
	"kAudioUnitProperty_ParameterClumpName",
	"kAudioUnitProperty_PresentPreset",
	"kAudioUnitProperty_OfflineRender",
	"kAudioUnitProperty_ParameterValueFromString",
	"kAudioUnitProperty_IconLocation",
	"kAudioUnitProperty_PresentationLatency",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"kAudioUnitProperty_DependentParameters",
	"kAudioUnitProperty_AUHostIdentifier",
	"kAudioUnitProperty_MIDIOutputCallbackInfo",
	"kAudioUnitProperty_MIDIOutputCallback",
	"kAudioUnitProperty_InputSamplesInOutput",
	"kAudioUnitProperty_ClassInfoFromDocument",
	"kAudioUnitProperty_ShouldAllocateBuffer",
	"kAudioUnitProperty_FrequencyResponse",
	"kAudioUnitProperty_ParameterHistoryInfo",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
	"{unknown}",
};
#else
const char * _Nonnull AudioUnitImpl::kAudioUnitPropertyNames[] = {"{UNKNOWN}"};
#endif

REFLEX_END_INTERNAL

extern "C" __attribute__((visibility("default"))) void * _Null_unspecified AudioUnitFactory(const AudioComponentDescription * _Null_unspecified inDesc);

extern "C" __attribute__((visibility("default"))) void * _Null_unspecified AudioUnitFactory(const AudioComponentDescription * _Null_unspecified inDesc)
{
	REFLEX_USE(Reflex);

	REFLEX_LOCAL(OSStatus,AUEntryOpen)(void * _Nonnull ptr, AudioComponentInstance _Nonnull ci)
	{
		auto au = Reflex::System::OSX::TheAudioUnitSession::Acquire();

		Retain(au);

		auto & acpi = *reinterpret_cast<System::OSX::AudioUnitImpl::ACPI*>(ptr);

		auto & session = au->m_session;

		new (&acpi.storage) System::OSX::AudioUnitImpl(session, session.config.classes.GetFirst(), ci);

		return noErr;
	}
	REFLEX_END;

	REFLEX_LOCAL(OSStatus,AUEntryClose)(void * _Nonnull ptr)
	{
		auto & acpi = *reinterpret_cast<System::OSX::AudioUnitImpl::ACPI*>(ptr);

		Reinterpret<System::OSX::AudioUnitImpl>(&acpi.storage)->~AudioUnitImpl();

		Release(*Reflex::System::OSX::TheAudioUnitSession::Get());

		return noErr;
	}
	REFLEX_END;

	REFLEX_ASSERT_MAINTHREAD("AudioUnitImpl::AUEntryOpen");

	void * ptr = REFLEX_ALLOC16(offsetof(System::OSX::AudioComponentPlugInInstance,storage) + sizeof(System::OSX::AudioUnitImpl));

	auto & acpi = *reinterpret_cast<System::OSX::AudioComponentPlugInInstance*>(ptr);

	acpi.plugininterface.Open = &AUEntryOpen::Call;

	acpi.plugininterface.Close = &AUEntryClose::Call;

	acpi.plugininterface.Lookup = &System::OSX::AudioUnitImpl::AUEntryLookup;

	acpi.plugininterface.reserved = 0;
	acpi.a = 0;
	acpi.b = 0 ;
	acpi.pad[0] = 0;
	acpi.pad[1] = 0;

	return ptr;
}

#define AU_LEGACY_METHOD_PARAMS System::OSX::AudioUnitImpl & au, ComponentParameters * _Null_unspecified params

extern "C" __attribute__((visibility("default"))) ComponentResult AudioUnitEntry(ComponentParameters * _Null_unspecified params, Handle _Null_unspecified handle);

REFLEX_DISABLE_WARNINGS
extern "C" __attribute__((visibility("default"))) ComponentResult AudioUnitEntry(ComponentParameters * _Null_unspecified params, Handle _Null_unspecified handle)
{
	REFLEX_USE(Reflex);

	typedef System::OSX::AudioUnitImpl AudioUnit;

	REFLEX_LOCAL(ComponentResult,AudioUnitGetPropertyInfo)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			AudioUnitPropertyID propertyid;
			UInt32 pad1;
			AudioUnitScope scope;
			UInt32 pad2;
			AudioUnitElement element;
			UInt32 pad3;
			UInt32 * datasize;
			Boolean * writable;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		bool writable = false;

		UInt32 size = 0;

		if (au.GetPropertyInfo(data.propertyid, data.scope, data.element, writable, size))
		{
			if (data.writable != 0) (*data.writable) = writable;

			if (data.datasize != 0) (*data.datasize) = size;

			return noErr;
		}
		else
		{
			return kAudioUnitErr_InvalidProperty;
		}
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitGetProperty)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			AudioUnitPropertyID propertyid;
			UInt32 pad1;
			AudioUnitScope scope;
			UInt32 pad2;
			AudioUnitElement element;
			UInt32 pad3;
			void * pdata;
			UInt32 * datasize;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		return au.GetProperty(data.propertyid, data.scope, data.element, data.pdata, *data.datasize);
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitSetProperty)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			AudioUnitPropertyID propertyid;
			UInt32 pad1;
			AudioUnitScope scope;
			UInt32 pad2;
			AudioUnitElement element;
			UInt32 pad3;
			void * pdata;
			UInt32 datasize;
			UInt32 pad4;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		return au.SetProperty(data.propertyid, data.scope, data.element, data.pdata, data.datasize);
	}
	REFLEX_END

	struct PropertyListenerData
	{
		UInt64 componentinfo;
		::AudioUnit audiounit;
		AudioUnitPropertyID propertyid;
		UInt32 pad1;
		AudioUnitPropertyListenerProc callback;
		void * client;
	};

	REFLEX_LOCAL(ComponentResult,AudioUnitAddPropertyListener)(AU_LEGACY_METHOD_PARAMS)
	{
		auto & data = *reinterpret_cast<PropertyListenerData*>(params);

		if (!au.m_propertylisteners.Search(data.propertyid))
		{
			auto & listener = au.m_propertylisteners.Insert(data.propertyid);

			listener.property = data.propertyid;
			listener.callback = data.callback;
			listener.client = data.client;
		}

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitRemovePropertyListener)(AU_LEGACY_METHOD_PARAMS)
	{
		auto & data = *reinterpret_cast<PropertyListenerData*>(params);

		if (auto idx = au.m_propertylisteners.Search(data.propertyid)) au.m_propertylisteners.Remove(idx.value);

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitGetParameter)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			AudioUnitParameterID parameterid;
			UInt32 pad1;
			AudioUnitScope scope;
			UInt32 pad2;
			AudioUnitElement element;
			UInt32 pad3;
			Float32 * pvalue;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		(*data.pvalue) = au.GetParameterImpl(data.parameterid);

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitSetParameter)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			AudioUnitParameterID parameterid;
			UInt32 pad1;
			AudioUnitScope scope;
			UInt32 pad2;
			AudioUnitElement element;
			UInt32 pad3;
			Float32 value;
			UInt32 pad4;
			UInt32 sampleoffset;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		au.UpdateParameter(data.parameterid, data.value, data.sampleoffset);

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitScheduleParameters)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			const AudioUnitParameterEvent * parameterevents;
			UInt32 numevents;
			UInt32 pad1;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		auto event = data.parameterevents;

		REFLEX_LOOP(idx, data.numevents)
		{
			if (event->eventType == kParameterEvent_Immediate)
			{
				auto & immediate = event->eventValues.immediate;

				au.UpdateParameter(event->parameter, immediate.value, immediate.bufferOffset);
			}

			event++;
		}

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitRender)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			UInt32 * flags;
			const AudioTimeStamp * timestamp;
			UInt32 bus;
			UInt32 pad1;
			UInt32 samples;
			UInt32 pad2;
			AudioBufferList * buffers;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		return AudioUnit::Render(au, data.flags, data.timestamp, data.bus, data.samples, data.buffers);
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,AudioUnitReset)(AU_LEGACY_METHOD_PARAMS)
	{
		au.m_rendertimestamp = -1.0;

		return noErr;
	}
	REFLEX_END

	REFLEX_LOCAL(ComponentResult,MusicDeviceMIDIEvent)(AU_LEGACY_METHOD_PARAMS)
	{
		struct Params
		{
			UInt64 componentinfo;
			::AudioUnit audiounit;
			UInt32 status;
			UInt32 pad1;
			UInt32 data1;
			UInt32 pad2;
			UInt32 data2;
			UInt32 pad3;
			UInt32 sample;
			UInt32 pad4;
		};

		auto & data = *reinterpret_cast<Params*>(params);

		UInt8 mididata[4] = {UInt8(data.status), UInt8(data.data1), UInt8(data.data2), 0};

		au.QueueEvent(AudioUnit::Event::kTypeMIDI, UInt16(data.sample), 0, *reinterpret_cast<UInt32*>(mididata));

		return noErr;
	}
	REFLEX_END

	auto & au = *reinterpret_cast<AudioUnit*>(handle);

	switch (params->what)
	{
		case kAudioUnitRenderSelect:
			return AudioUnitRender::Call(au, params);

		case kMusicDeviceMIDIEventSelect:
			return MusicDeviceMIDIEvent::Call(au, params);


		case kAudioUnitGetParameterSelect:
			return AudioUnitGetParameter::Call(au, params);

		case kAudioUnitSetParameterSelect:
			return AudioUnitSetParameter::Call(au, params);

		case kAudioUnitScheduleParametersSelect:
			return AudioUnitScheduleParameters::Call(au, params);


		case kAudioUnitResetSelect:
			return AudioUnitReset::Call(au, params);

		case kAudioUnitGetPropertyInfoSelect:
			return AudioUnitGetPropertyInfo::Call(au, params);

		case kAudioUnitGetPropertySelect:
			return AudioUnitGetProperty::Call(au, params);

		case kAudioUnitSetPropertySelect:
			return AudioUnitSetProperty::Call(au, params);


		case kAudioUnitAddRenderNotifySelect:
			return noErr;

		case kAudioUnitRemoveRenderNotifySelect:
			return noErr;


		case kAudioUnitAddPropertyListenerSelect:
			return AudioUnitAddPropertyListener::Call(au, params);

		case kAudioUnitRemovePropertyListenerSelect:
			return AudioUnitRemovePropertyListener::Call(au, params);

		case kAudioUnitRemovePropertyListenerWithUserDataSelect:
			return AudioUnitRemovePropertyListener::Call(au, params);


		case kComponentCanDoSelect:
			switch (params->params[0])
			{
				case kAudioUnitInitializeSelect:
				case kAudioUnitUninitializeSelect:
				case kAudioUnitGetPropertyInfoSelect:
				case kAudioUnitGetPropertySelect:
				case kAudioUnitSetPropertySelect:
				case kAudioUnitAddPropertyListenerSelect:
				case kAudioUnitRemovePropertyListenerSelect:
				case kAudioUnitGetParameterSelect:
				case kAudioUnitSetParameterSelect:
				case kAudioUnitResetSelect:
				case kAudioUnitScheduleParametersSelect:
				case kAudioUnitRenderSelect:
				case kMusicDeviceMIDIEventSelect:
				case kComponentOpenSelect:
				case kComponentCloseSelect:
				case kComponentVersionSelect:
				case kComponentCanDoSelect:
					return 1;
			}
			return 0;

		case kAudioUnitInitializeSelect:
			return noErr;
		case kAudioUnitUninitializeSelect:
			return noErr;

		case kComponentOpenSelect:
		{
			auto auComponentInstance = reinterpret_cast<ComponentInstance>(params->params[0]);

			auto au = Reflex::System::OSX::TheAudioUnitSession::Acquire();

			Retain(au);

			auto & session = au->m_session;

			auto audiounit = REFLEX_CREATE(AudioUnit, session, session.config.classes.GetFirst(), auComponentInstance);

			SetComponentInstanceStorage(auComponentInstance, reinterpret_cast<Handle>(audiounit));

			return noErr;
		}

		case kComponentCloseSelect:
		{
			auto auComponentInstance = reinterpret_cast<ComponentInstance>(params->params[0]);

			auto audiounit = reinterpret_cast<AudioUnit*>(handle);

			Release(*audiounit);

			SetComponentInstanceStorage(auComponentInstance, 0);

			Release(*Reflex::System::OSX::TheAudioUnitSession::Get());

			return noErr;
		}

		case kComponentVersionSelect:
			return 0x10000;

		default:
			return badComponentSelector;
	}
}
REFLEX_ENABLE_WARNINGS

@implementation NS_PluginView

- (id _Nonnull) init:(Reflex::System::OSX::AudioUnitImpl * _Nonnull)instance
{
	if ((self = [self initWithFrame:NSMakeRect(0, 0, 0, 0)]))
	{
		m_instance = instance;
	}

	return self;
}

- (void) dealloc
{
	m_instance->DiscardEditor();
	
	#if !__has_feature(objc_arc)
	[super dealloc];

	self = 0;
	#endif
}

- (BOOL) isOpaque{return true;}
- (BOOL) isFlipped{return false;}
- (BOOL) acceptsFirstResponder{return YES;}
- (BOOL) acceptsFirstMouse:(NSEvent * _Nullable)event {return YES;}

- (void) rightMouseDown:(NSEvent * _Nonnull)event;
{
	[[self nextResponder] rightMouseDown:event];
}

- (void) viewWillMoveToSuperview:(NSView * _Nullable) superview;
{
	if (!superview)
	{
		m_instance->DiscardEditor();
	}
}

@end
