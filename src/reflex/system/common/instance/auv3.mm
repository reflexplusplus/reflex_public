#include "plugin.h"
#include "instance.cpp"
#include "plugin.cpp"

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

typedef Reflex::The<PluginSession> ThePluginSession;

class Auv3Instance : public PluginInstance
{
public:
	REFLEX_OBJECT(Auv3Instance, PluginInstance);

	Auv3Instance(PluginSession & session, const Configuration::Class & cls)
		: PluginInstance(session, cls)
		, m_rendering(false)
		, m_latency(0)
		, m_hostview(nullptr)
		, m_midioutputblock(nullptr)
		, m_parametertree(nullptr)
		, m_editorobservertoken(nullptr)
	{
		Initialise();
	}

	~Auv3Instance()
	{
		DiscardEditor();
		Deinitialise();
	}

	virtual void OnSetViewSize(const iSize & size) override
	{
		m_onresize(size);
	}

	virtual void ReportProcessingDelay(UInt32 delay) override
	{
		m_latency = delay;
	}

	virtual void ReportStateChange(UInt8 change_flags) override
	{
		// Label/range refresh (kChangeParameterInfo) would require a full
		// AUParameterTree rebuild — deliberately not done (fixed-pool model;
		// see design doc). Values are pushed regardless, which covers the
		// common case (e.g. an fx-type switch changing parameter values).
		if (change_flags & (kChangeParameterValues | kChangeParameterInfo))
		{
			auto & client = GetCallbacks();
			for (UInt32 idx = 0; idx < cls.num_params; ++idx)
			{
				AUParameter * param = [m_parametertree parameterWithAddress:idx];
				[param setValue:client.OnGetParameterValue(idx) originator:m_editorobservertoken];
			}
		}
	}

	virtual void BeginAutomation(UInt32 idx) override
	{
		AUParameter * param = [m_parametertree parameterWithAddress:idx];
		[param setValue:param.value originator:m_editorobservertoken atHostTime:0 eventType:AUParameterAutomationEventTypeTouch];
	}

	virtual void Automate(UInt32 idx, Float32 value) override
	{
		AUParameter * param = [m_parametertree parameterWithAddress:idx];
		[param setValue:value originator:m_editorobservertoken atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
	}

	virtual void EndAutomation(UInt32 idx) override
	{
		AUParameter * param = [m_parametertree parameterWithAddress:idx];
		[param setValue:param.value originator:m_editorobservertoken atHostTime:0 eventType:AUParameterAutomationEventTypeRelease];
	}

	void SetParameterTree(AUParameterTree * tree, AUParameterObserverToken token)
	{
		m_parametertree = tree;
		m_editorobservertoken = token;
	}

	void SetHostView(void * view, Function<void(const iSize &)> onResize)
	{
		m_hostview = view;
		m_onresize = onResize;
		if (m_hostview)
		{
			ShowEditor(m_hostview);
		}
	}

	void ClearHostView()
	{
		DiscardEditor();
		m_hostview = nullptr;
		m_onresize.Clear();
	}

	void SetMIDIOutputBlock(AUMIDIOutputEventBlock block)
	{
		m_midioutputblock = block;
	}

	void SyncHostParam(UInt32 idx, Float32 value)
	{
		GetCallbacks().OnSetParameterValue(idx, value);
	}

	Float32 GetParam(UInt32 idx)
	{
		return GetCallbacks().OnGetParameterValue(idx);
	}

	void GetParamInfo(UInt32 idx, CString & name)
	{
		GetCallbacks().OnGetParameterInfo(idx, name);
	}

	UInt32 GetLatency() const { return m_latency; }

	void StartRender(UInt32 maxFrames, Float64 sampleRate)
	{
		PrepareProcessing(maxFrames, Float32(sampleRate));
		m_rendering = true;
	}

	void StopRender()
	{
		m_rendering = false;
	}

	OSStatus Render(AudioBufferList * outputData,
		AUAudioFrameCount frameCount,
		const AURenderEvent * realtimeEventListHead,
		AURenderPullInputBlock pullInputBlock,
		AUHostMusicalContextBlock musicalContext)
	{
		if (!m_rendering)
		{
			return noErr;
		}

		// --- output channels
		auto outs = GetAudioChannels(true);
		for (UInt32 i = 0; i < outs.size && i < outputData->mNumberBuffers; ++i)
		{
			outs[i] = reinterpret_cast<Float32*>(outputData->mBuffers[i].mData);
		}

		// --- input channels: pull into a separately allocated buffer list
		auto ins = GetAudioChannels(false);
		if (ins.size && pullInputBlock)
		{
			// Allocate a scratch ABL on the stack mirroring the output layout
			// (same channel count / buffer sizes; mData pointers will be filled
			// by the host during the pull call).
			const UInt32 nBufs = outputData->mNumberBuffers;
			const UInt32 ablSize = sizeof(AudioBufferList) + (nBufs - 1) * sizeof(AudioBuffer);
			AudioBufferList * inABL = reinterpret_cast<AudioBufferList*>(alloca(ablSize));
			inABL->mNumberBuffers = nBufs;
			for (UInt32 i = 0; i < nBufs; ++i)
			{
				inABL->mBuffers[i] = outputData->mBuffers[i];
				inABL->mBuffers[i].mData = nullptr; // host will fill
			}

			AudioUnitRenderActionFlags actionFlags = 0;
			AudioTimeStamp timestamp = {};
			timestamp.mFlags = kAudioTimeStampSampleTimeValid;
			timestamp.mSampleTime = 0;

			pullInputBlock(&actionFlags, &timestamp, frameCount, 0, inABL);

			for (UInt32 i = 0; i < ins.size && i < inABL->mNumberBuffers; ++i)
			{
				ins[i] = reinterpret_cast<Float32*>(inABL->mBuffers[i].mData);
			}
		}

		// --- RT events
		for (auto ev = realtimeEventListHead; ev != nullptr; ev = ev->head.next)
		{
			if (ev->head.eventType == AURenderEventMIDI)
			{
				auto & m = ev->MIDI;
				UInt32 msg = UInt32(m.data[0]) | (UInt32(m.data[1]) << 8) | (UInt32(m.data[2]) << 16);
				QueueEvent(Event::kTypeMIDI, UInt16(Min<UInt32>(m.eventSampleTime, 0x3FFF)), 0, msg);
			}
			else if (ev->head.eventType == AURenderEventParameter || ev->head.eventType == AURenderEventParameterRamp)
			{
				auto & p = ev->parameter;
				if (p.parameterAddress < cls.num_params)
				{
					QueueEvent(Event::kTypeAutomation, UInt16(Min<UInt32>(p.eventSampleTime, 0x3FFF)), UInt16(p.parameterAddress), Float32(p.value));
				}
			}
		}

		// --- transport
		bool clock = false;
		Float64 bps = 1.0;
		Float64 beatpos = 0.0;

		if (musicalContext)
		{
			// AUHostMusicalContextBlock signature: (currentTempo,
			// timeSignatureNumerator [double], timeSignatureDenominator
			// [NSInteger], currentBeatPosition, sampleOffsetToNextBeat
			// [NSInteger], currentMeasureDownbeatPosition). It carries no
			// transport play-state — that comes from the transport-state
			// block, which this render path does not receive, so `clock`
			// stays at its default.
			double tempo = 120.0;
			double numerator = 4.0;
			NSInteger denominator = 4;
			double beatPosition = 0.0;
			NSInteger sampleOffsetToNextBeat = 0;
			double currentMeasureDownbeat = 0.0;

			if (musicalContext(&tempo, &numerator, &denominator, &beatPosition, &sampleOffsetToNextBeat, &currentMeasureDownbeat))
			{
				bps = Max(tempo, 1.0) * kRcpSixty;
				beatpos = beatPosition;
			}
		}

		auto evOut = ProcessRt(frameCount, clock, bps, beatpos);

		// --- MIDI output
		if (m_midioutputblock && cls.midi_io.b)
		{
			REFLEX_FOREACH(e, evOut)
			{
				if (e.type == Event::kTypeMIDI)
				{
					UInt8 bytes[3] = {
						UInt8(e.value.u32 & 0xFF),
						UInt8((e.value.u32 >> 8) & 0xFF),
						UInt8((e.value.u32 >> 16) & 0xFF)
					};
					m_midioutputblock(AUEventSampleTimeImmediate + e.position, 0, 3, bytes);
				}
			}
		}

		return noErr;
	}

	Array<UInt8> GetChunk() { return GetCallbacks().OnGetPluginChunk(); }
	void SetChunk(const ArrayView<UInt8> & chunk) { GetCallbacks().OnSetPluginChunk(chunk); }

private:
	bool m_rendering;
	UInt32 m_latency;
	void * m_hostview;
	AUMIDIOutputEventBlock m_midioutputblock;
	Function<void(const iSize &)> m_onresize;
	AUParameterTree * m_parametertree;
	AUParameterObserverToken m_editorobservertoken;
};

TRef<PluginSession> AcquireAuv3Session()
{
	return ThePluginSession::Acquire(nullptr, kPluginFormatAUv3);
}

REFLEX_END_INTERNAL

#if TARGET_OS_IPHONE
@interface ReflexAuv3ViewController : UIViewController
#else
@interface ReflexAuv3ViewController : NSViewController
#endif
{
@public
	Reflex::System::Common::Auv3Instance * _instance;
}
@end

@implementation ReflexAuv3ViewController

#if TARGET_OS_IPHONE
- (void)viewDidLoad
{
	[super viewDidLoad];
	if (_instance)
	{
		__weak ReflexAuv3ViewController * weakSelf = self;
		_instance->SetHostView((__bridge void*)self.view, [weakSelf](const Reflex::System::iSize & size)
		{
			if (auto * vc = weakSelf)
			{
				vc.preferredContentSize = CGSizeMake(size.w, size.h);
				vc.view.frame = CGRectMake(0, 0, size.w, size.h);
			}
		});
	}
}
#else
- (void)loadView
{
	self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 256, 256)];
	if (_instance)
	{
		__weak ReflexAuv3ViewController * weakSelf = self;
		_instance->SetHostView((__bridge void*)self.view, [weakSelf](const Reflex::System::iSize & size)
		{
			if (auto * vc = weakSelf)
			{
				vc.preferredContentSize = NSMakeSize(size.w, size.h);
				vc.view.frame = NSMakeRect(0, 0, size.w, size.h);
			}
		});
	}
}
#endif

- (void)dealloc
{
	if (_instance)
	{
		_instance->ClearHostView();
	}
#if !__has_feature(objc_arc)
	[super dealloc];
#endif
}

@end

@interface ReflexAuv3AudioUnit : AUAudioUnit
@end

@implementation ReflexAuv3AudioUnit
{
	AUAudioUnitBus * _inputBus;
	AUAudioUnitBus * _outputBus;
	AUAudioUnitBusArray * _inputBusses;
	AUAudioUnitBusArray * _outputBusses;
	AUParameterTree * _parameterTree;
	AUParameterObserverToken _editorObserverToken;
	Reflex::System::Common::Auv3Instance * _instance;
	BOOL _hasMidiInput;
	BOOL _hasMidiOutput;
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
							   options:(AudioComponentInstantiationOptions)options
								 error:(NSError **)outError
{
	self = [super initWithComponentDescription:componentDescription options:options error:outError];
	if (self)
	{
		auto session = Reflex::System::Common::AcquireAuv3Session();
		if (!session->config.classes)
		{
			return nil;
		}

		// AUv3 supports one class per AU extension — always use the first
		auto & cls = session->config.classes.GetFirst();
		_instance = new Reflex::System::Common::Auv3Instance(*session, cls);

		_hasMidiInput  = cls.midi_io.a ? YES : NO;
		_hasMidiOutput = cls.midi_io.b ? YES : NO;

		const bool isInstrument = (cls.type == Reflex::System::AudioPlugin::Configuration::Class::kTypeInstrument);

		// Output bus — always present
		AVAudioFormat * outFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100
		                                                                           channels:Reflex::Max<UInt32>(1,cls.channels_io.b)];
		_outputBus = [[AUAudioUnitBus alloc] initWithFormat:outFormat error:outError];
		_outputBusses = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
		                                                       busType:AUAudioUnitBusTypeOutput
		                                                        busses:@[_outputBus]];

		// Input bus — omit for instruments (they generate audio, not process it)
		if (!isInstrument && cls.channels_io.a > 0)
		{
			AVAudioFormat * inFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100
			                                                                          channels:Reflex::Max<UInt32>(1,cls.channels_io.a)];
			_inputBus = [[AUAudioUnitBus alloc] initWithFormat:inFormat error:outError];
			_inputBusses = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
			                                                       busType:AUAudioUnitBusTypeInput
			                                                        busses:@[_inputBus]];
		}
		else
		{
			_inputBusses = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
			                                                       busType:AUAudioUnitBusTypeInput
			                                                        busses:@[]];
		}

		// Parameters
		NSMutableArray<AUParameter*> * params = [NSMutableArray array];
		for (UInt32 i = 0; i < cls.num_params; ++i)
		{
			Reflex::CString pname;
			_instance->GetParamInfo(i, pname);
			NSString * identifier = [NSString stringWithFormat:@"p%u", i];
			NSString * name = [NSString stringWithUTF8String:pname.GetData()];
			AUParameter * p = [AUParameterTree createParameterWithIdentifier:identifier
			                                                             name:name
			                                                          address:i
			                                                              min:0.0
			                                                              max:1.0
			                                                             unit:kAudioUnitParameterUnit_Generic
			                                                         unitName:nil
			                                                            flags:kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable
			                                                     valueStrings:nil
			                                              dependentParameters:nil];
			p.value = _instance->GetParam(i);
			[params addObject:p];
		}

		_parameterTree = [AUParameterTree createTreeWithChildren:params];

		__weak ReflexAuv3AudioUnit * weakSelf = self;
		_parameterTree.implementorValueObserver = ^(AUParameter * param, AUValue value) {
			if (auto * au = weakSelf) { au->_instance->SyncHostParam(UInt32(param.address), value); }
		};
		_parameterTree.implementorValueProvider = ^AUValue(AUParameter * param) {
			if (auto * au = weakSelf) { return au->_instance->GetParam(UInt32(param.address)); }
			return 0.0f;
		};

		// Empty-block observer token used purely as a stable `originator:` handle
		// for the plugin's own automation writes (matches JUCE/iPlug2).
		_editorObserverToken = [_parameterTree tokenByAddingParameterObserver:^(AUParameterAddress, AUValue){}];
		_instance->SetParameterTree(_parameterTree, _editorObserverToken);
	}
	return self;
}

- (void)dealloc
{
	if (_editorObserverToken)
	{
		[_parameterTree removeParameterObserver:_editorObserverToken];
		_editorObserverToken = nullptr;
	}
	if (_instance)
	{
		delete _instance;
		_instance = nullptr;
	}
#if !__has_feature(objc_arc)
	[super dealloc];
#endif
}

- (AUAudioUnitBusArray *)inputBusses  { return _inputBusses; }
- (AUAudioUnitBusArray *)outputBusses { return _outputBusses; }
- (AUParameterTree *)parameterTree    { return _parameterTree; }

- (BOOL)canProcessInPlace { return NO; }

- (NSArray<NSString *> *)MIDIOutputNames
{
	return _hasMidiOutput ? @[@"MIDI Out"] : @[];
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError
{
	if (![super allocateRenderResourcesAndReturnError:outError])
	{
		return NO;
	}
	if (_instance)
	{
		_instance->StartRender(self.maximumFramesToRender, _outputBus.format.sampleRate);
		if (_hasMidiOutput)
		{
			_instance->SetMIDIOutputBlock(self.MIDIOutputEventBlock);
		}
	}
	return YES;
}

- (void)deallocateRenderResources
{
	if (_instance)
	{
		_instance->SetMIDIOutputBlock(nil);
		_instance->StopRender();
	}
	[super deallocateRenderResources];
}

- (AUInternalRenderBlock)internalRenderBlock
{
	__weak ReflexAuv3AudioUnit * weakSelf = self;
	return ^AUAudioUnitStatus(AudioUnitRenderActionFlags * actionFlags,
	                          const AudioTimeStamp * timestamp,
	                          AUAudioFrameCount frameCount,
	                          NSInteger outputBusNumber,
	                          AudioBufferList * outputData,
	                          const AURenderEvent * realtimeEventListHead,
	                          AURenderPullInputBlock pullInputBlock)
	{
		(void)actionFlags;
		(void)timestamp;
		(void)outputBusNumber;
		auto * au = weakSelf;
		if (!au || !au->_instance)
		{
			return noErr;
		}
		return au->_instance->Render(outputData, frameCount, realtimeEventListHead,
		                             pullInputBlock, au.musicalContextBlock);
	};
}

- (NSDictionary<NSString *,id> *)fullState
{
	if (!_instance) { return @{}; }
	auto chunk = _instance->GetChunk();
	NSData * data = [NSData dataWithBytes:chunk.GetData() length:chunk.GetSize()];
	return @{ @"ReflexChunk" : data };
}

- (void)setFullState:(NSDictionary<NSString *,id> *)fullState
{
	NSData * data = fullState[@"ReflexChunk"];
	if (_instance && data)
	{
		Reflex::ArrayView<Reflex::UInt8> chunk = { (Reflex::UInt8*)data.bytes, Reflex::UInt32(data.length) };
		_instance->SetChunk(chunk);
	}
}

- (void)requestViewControllerWithCompletionHandler:(void (^)(AUViewControllerBase * _Nullable))completionHandler
{
	ReflexAuv3ViewController * vc = [ReflexAuv3ViewController new];
	vc->_instance = _instance;
	completionHandler(vc);
}

@end

extern "C"
{
	Class ReflexAUv3AudioUnitClass()
	{
		return [ReflexAuv3AudioUnit class];
	}
}
