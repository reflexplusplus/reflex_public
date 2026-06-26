#import <AVFoundation/AVFoundation.h>

// Custom Audio Effect Node
@interface ReflexAudioEffectNode : AVAudioUnitEffect

- (instancetype)initWithFormat:(AVAudioFormat*)format renderBlock:(AUInternalRenderBlock)renderBlock;

@end

// Custom Audio Unit
@interface ReflexAudioEffectNodeAU : AUAudioUnit

@property (nonatomic, readonly) AUAudioUnitBus *inputBus;
@property (nonatomic, readonly) AUAudioUnitBus *outputBus;

@end
