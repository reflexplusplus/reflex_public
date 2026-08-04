#include "ReflexAudioEffectNode.h"



REFLEX_BEGIN_INTERNAL(Reflex::System::iOS)

struct AudioEffectNodeGlobals {
	AVAudioFormat* format;
	AUInternalRenderBlock renderBlock;
	BOOL hasRegisteredComponent = NO;
} g_AudioEffectNodeGlobals;

REFLEX_END_INTERNAL

@implementation ReflexAudioEffectNode

- (instancetype)initWithFormat:(AVAudioFormat*)format renderBlock:(AUInternalRenderBlock)renderBlock {
	AudioComponentDescription description = {
		.componentType = kAudioUnitType_Effect,
		.componentSubType = 'llap', // low latency audio processor
		.componentManufacturer = 'rflx', // ReflexMultimedia
		.componentFlags = 0,
		.componentFlagsMask = 0
	};

	auto& globals = Reflex::System::iOS::g_AudioEffectNodeGlobals;

	if (!globals.hasRegisteredComponent) {
		globals.hasRegisteredComponent = YES;
		[AUAudioUnit registerSubclass:ReflexAudioEffectNodeAU.class
			   asComponentDescription:description
								 name:@"AVAudioEffectNode"
							  version:0];
	}

	globals.format = format;
	globals.renderBlock = [renderBlock copy];

	return (self = [super initWithAudioComponentDescription:description]);
}

@end

@implementation ReflexAudioEffectNodeAU {
	AUAudioUnitBus* _inputBus;
	AUAudioUnitBus* _outputBus;
	AUInternalRenderBlock _internalRenderBlock;
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
								   options:(AudioComponentInstantiationOptions)options
									 error:(NSError**)outError {
	self = [super initWithComponentDescription:componentDescription options:options error:outError];
	if (self) {
		auto& globals = Reflex::System::iOS::g_AudioEffectNodeGlobals;
		auto* audioFormat = globals.format;
		_internalRenderBlock = [globals.renderBlock copy];

		globals.format = nil;
		globals.renderBlock = nil;

		_inputBus = [[AUAudioUnitBus alloc] initWithFormat:audioFormat error:outError];
		if (!_inputBus) return nil;

		_outputBus = [[AUAudioUnitBus alloc] initWithFormat:audioFormat error:outError];
		if (!_outputBus) return nil;
	}
	return self;
}

- (AUAudioUnitBusArray*)inputBusses {
	return [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeInput busses:@[_inputBus]];
}

- (AUAudioUnitBusArray*)outputBusses {
	return [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses:@[_outputBus]];
}

- (AUInternalRenderBlock)internalRenderBlock {
	return _internalRenderBlock;
}

@end
