#pragma once

#include "../sdk.h"




//
//declarations

REFLEX_NS(Reflex::System::iOS)

typedef Function<void(const WString&, Pair<UInt>)> OnTextInputDoneCallback;

REFLEX_END




//
//ReflexViewController

@interface ReflexViewController : UIViewController<UIGestureRecognizerDelegate, UITextFieldDelegate, UITextViewDelegate> {}

- (UIEdgeInsets)viewInsets;

- (void)viewSizeDidChange;

@end

@interface ReflexViewController(Keyboard)

- (void)beginTextInput:(Reflex::System::VirtualKeyboardInputType)type
			  withText:(NSString*)text
			 selection:(Reflex::Pair<Reflex::UInt>)selection
				onDone:(const Reflex::System::iOS::OnTextInputDoneCallback&)callback;

- (void)dismissTextInput;

- (void)showKeyboard:(BOOL)shown;

@end
