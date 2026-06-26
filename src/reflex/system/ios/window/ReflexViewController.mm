#import "ReflexViewController.h"
#include "../app.h"




REFLEX_BEGIN_INTERNAL(Reflex::System::iOS)

fPoint ToPoint(CGPoint point) {
	return { float(point.x), float(point.y) };
}

REFLEX_END_INTERNAL

///
/// Special view meant to handle a copy of the view when the application goes in background (and we destroy the view).
///
@interface UIView(Snapshot)

- (nullable UIImage*)takeSnapshot;

@end

@interface ReflexTextFieldForKeyboardInput : UITextView {}
@end

@interface ReflexTextField : UITextField {}

- (id)initWithFrame:(CGRect)frame delegate:(id<UITextFieldDelegate>)delegate;

- (void)setInputType:(Reflex::System::VirtualKeyboardInputType)type;

@end

///
/// ReflexView controller declarations.
///
@interface ReflexViewController(/* Private */) {
	struct TouchInfo
	{
		int flingDirection = -1;
		UIKeyModifierFlags modifierFlags = 0;
		BOOL isLongPress = NO, hasPanned = NO;
	} m_touchInfo;

	BOOL m_keyboardShown;
	CGRect m_keyboardRect;
	// Single line
	ReflexTextField* m_inputview;
	ReflexTextFieldForKeyboardInput* m_keyboardinputview;
	NSString* m_currentInputText;
	// Multiline
	UITextView* m_textEditor;
	UIToolbar* m_textEditorToolbar;
	Reflex::System::iOS::OnTextInputDoneCallback m_onTextDone;
}
@end

///
/// Special view meant to handle a copy of the view when the application goes in background (and we destroy the view).
///

@implementation ReflexViewController(Keyboard)

- (void)beginTextInput:(Reflex::System::VirtualKeyboardInputType)type
			  withText:(nonnull NSString*)text
			 selection:(Reflex::Pair<Reflex::UInt>)selection
				onDone:(const Reflex::System::iOS::OnTextInputDoneCallback&)callback
{
	// Config change that requires reopening
	bool multiline = type == Reflex::System::kVirtualKeyboardInputMultiLine;
	if ((m_textEditor && !multiline) || (m_inputview && multiline)) {
		[self dismissTextInput];
	}

	m_onTextDone = callback;

	if (multiline) {
		if (!m_textEditor) {
			m_textEditor = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
			m_textEditor.delegate = self;
			m_textEditor.userInteractionEnabled = YES;

			[self.view addSubview:m_textEditor];
		}

		m_textEditor.text = text;
		[m_textEditor select:m_textEditor];
		[m_textEditor setSelectedRange:NSMakeRange(selection.a, selection.b)];

		if (!m_textEditorToolbar) {
			m_textEditorToolbar = [UIToolbar new];
			m_textEditorToolbar.barStyle = UIBarStyleDefault;

			auto items = @[
				[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(cancelPressed:)],
				[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil],
				[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(donePressed:)],
			];
			[m_textEditorToolbar setItems:items animated:NO];

			[self.view addSubview:m_textEditorToolbar];
		}

		m_textEditorToolbar.frame = CGRectMake(0, 0, self.view.frame.size.width, 50);
		[self layoutTextEditorViews];

		[self.view.window makeKeyAndVisible];
		[m_textEditor becomeFirstResponder];
	}
	else {
		if (!m_inputview) {
			m_inputview = [[ReflexTextField alloc] initWithFrame:CGRectMake(0, 0, 1, 1) delegate:self];
			[self.view addSubview:m_inputview];
		}

		m_inputview.text = text;
//		[m_inputview select:m_inputview];
		[m_inputview setInputType:type];

		[m_inputview addTarget:self action:@selector(textFieldDidChange:) forControlEvents:UIControlEventEditingChanged];

		[self layoutTextEditorViews];

		[self.view.window makeKeyAndVisible];
		[m_inputview becomeFirstResponder];

		auto start = [m_inputview positionFromPosition:m_inputview.beginningOfDocument offset:selection.a];
		auto end = [m_inputview positionFromPosition:m_inputview.beginningOfDocument offset:selection.a + selection.b];
		[m_inputview setSelectedTextRange:[m_inputview textRangeFromPosition:start toPosition:end]];
	}
}

- (void)cancelPressed:(nonnull id)sender {
	auto callback = m_onTextDone;		//@FLORIAN -> WHATS THIS
	[self dismissTextInput];
}

- (void)donePressed:(nonnull id)sender {
	auto callback = m_onTextDone;
	auto t = m_textEditor;
	auto text = t.text;
	auto start = [t offsetFromPosition:t.beginningOfDocument toPosition:t.selectedTextRange.start];
	auto end = [t offsetFromPosition:t.beginningOfDocument toPosition:t.selectedTextRange.end];
	[self dismissTextInput];

	callback(Reflex::System::ToWString(text), { Reflex::UInt(start), Reflex::UInt(end - start) });
}

- (void)dismissTextInput {
	if (m_textEditor) {
		[m_textEditor removeFromSuperview];
		m_textEditor = nil;

		[m_textEditorToolbar removeFromSuperview];
		m_textEditorToolbar = nil;
	}
	else if (m_inputview) {
		m_currentInputText = @"";

		[m_inputview removeFromSuperview];
		m_inputview = nil;
	}

	m_onTextDone.Clear();
}

- (BOOL)isUsingNativeTextInputBox {
	return [m_inputview isFirstResponder] || [m_textEditor isFirstResponder] || [m_keyboardinputview isFirstResponder];
}

- (void)layoutTextEditorViews {
	auto screenSize = UIScreen.mainScreen.bounds.size;

	if (m_textEditor) {
		auto insets = [self viewInsets];

		m_textEditor.frame = CGRectMake(
			insets.left,
			insets.top,
			screenSize.width - insets.left - insets.right,
			screenSize.height - insets.bottom - insets.top - m_textEditorToolbar.frame.size.height);

		m_textEditorToolbar.frame = CGRectMake(
			m_textEditor.frame.origin.x,
			m_textEditor.frame.origin.y + m_textEditor.frame.size.height,
			m_textEditor.frame.size.width,
			m_textEditorToolbar.frame.size.height);
	}

	m_inputview.frame = CGRectMake(0, screenSize.height + 20, 100, 100);
	m_keyboardinputview.frame = CGRectMake(0, screenSize.height + 20, 100, 100);
}


- (void)keyboardWillChange:(nonnull NSNotification*)notification {
	const auto kMinKeyboardHeight = 10.0f;

	m_keyboardRect = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
	m_keyboardRect = [self.view convertRect:m_keyboardRect fromView:nil];
	// Unfortunately, keyboardWillChange is randomly fired after or before keyboardWillShow.
	m_keyboardShown = m_keyboardRect.size.height > kMinKeyboardHeight;

	[self viewSizeDidChange];
}

- (void)keyboardWillShow:(nonnull NSNotification*)notification {
	m_keyboardShown = YES;
}

- (void)keyboardWillHide:(nonnull NSNotification*)notification {
	m_keyboardShown = NO;
	m_keyboardRect = CGRectMake(0, 0, 0, 0);

	[self viewSizeDidChange];
}

- (void)showKeyboard:(BOOL)shown {
	if (shown) {
		[self->m_keyboardinputview becomeFirstResponder];
	}
	else {
		self->m_currentInputText = @"";
		[self->m_keyboardinputview setText:@""];
		[self->m_keyboardinputview resignFirstResponder];
	}
}

- (void)textFieldDidChange:(nonnull UITextField*)t {
	// Single line editor
	auto start = [t offsetFromPosition:t.beginningOfDocument toPosition:t.selectedTextRange.start];
	auto end = [t offsetFromPosition:t.beginningOfDocument toPosition:t.selectedTextRange.end];
	m_onTextDone(Reflex::System::ToWString(t.text), { Reflex::UInt(start), Reflex::UInt(end - start) });
}

- (BOOL)textFieldShouldReturn:(nonnull UITextField*)textField {
	auto callback = m_onTextDone;	//@FLORIAN -> WHATS THIS
	[self dismissTextInput];
	return YES;
}

- (void)textViewDidChange:(nonnull UITextView*)textView {
	if (textView != m_keyboardinputview) return;

	auto window = Reflex::System::iOS::Window::st_self;
	if (!window) return;

	auto lastText = m_currentInputText;
	m_currentInputText = textView.text;

	auto firstDifferentChar = lastText.length;
	for (auto i = 0U; i < lastText.length; i++) {
		if (i >= m_currentInputText.length
			|| [lastText characterAtIndex:i] != [m_currentInputText characterAtIndex:i])
		{
			firstDifferentChar = i;
			break;
		}
	}

	window->Internal_HandleImeInput(
		Reflex::UInt(lastText.length - firstDifferentChar),
		Reflex::System::ToWString([m_currentInputText substringFromIndex:firstDifferentChar]));
}

@end

@implementation ReflexViewController(GestureRecognizers)

- (void)_addRecognizer:(nonnull UIGestureRecognizer*)recognizer {
	recognizer.cancelsTouchesInView = NO;
	recognizer.delaysTouchesBegan = NO;
	recognizer.delaysTouchesEnded = NO;
	[self.view addGestureRecognizer:recognizer];
}

- (void)touchesBegan:(nonnull NSSet<UITouch*>*)touches withEvent:(nullable UIEvent*)event {
	auto window = Reflex::System::iOS::Window::st_self;
	auto* view = self.view;
	if (window && view) {
		for (UITouch* touch in touches) {
			window->GetClient()->OnTouchBegin(Reflex::ToUIntNative(touch), touch.timestamp, Reflex::System::iOS::ToPoint([touch locationInView:view]));
		}
	}

	[super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(nonnull NSSet<UITouch*>*)touches withEvent:(nullable UIEvent*)event {
	auto window = Reflex::System::iOS::Window::st_self;
	auto* view = self.view;
	if (window && view) {
		for (UITouch* touch in touches) {
			window->GetClient()->OnTouchMove(Reflex::ToUIntNative(touch), touch.timestamp, Reflex::System::iOS::ToPoint([touch locationInView:view]));
		}
	}

	[super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(nonnull NSSet<UITouch*>*)touches withEvent:(nullable UIEvent*)event {
	auto window = Reflex::System::iOS::Window::st_self;
	auto* view = self.view;
	if (window && view) {
		for (UITouch* touch in touches) {
			window->GetClient()->OnTouchEnd(Reflex::ToUIntNative(touch), touch.timestamp);
		}
	}

	[super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(nonnull NSSet<UITouch*>*)touches withEvent:(nullable UIEvent*)event {
	auto window = Reflex::System::iOS::Window::st_self;
	auto* view = self.view;
	if (window && view) {
		for (UITouch* touch in touches) {
			window->GetClient()->OnTouchCancel(Reflex::ToUIntNative(touch), touch.timestamp);
		}
	}

	[super touchesCancelled:touches withEvent:event];
}

@end


@implementation UIView (Snapshot)

- (nullable UIImage*)takeSnapshot {
	UIGraphicsBeginImageContextWithOptions(self.bounds.size, NO, [UIScreen mainScreen].scale);
	[self drawViewHierarchyInRect:self.bounds afterScreenUpdates:YES];
	UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return image;
}

@end

@implementation ReflexTextField

- (id)initWithFrame:(CGRect)frame delegate:(id<UITextFieldDelegate>)delegate {
	if ((self = [super initWithFrame:frame])) {
		self.delegate = delegate;
		self.returnKeyType = UIReturnKeyDone;
		self.layer.zPosition = -1; // backdrop
//		self.hidden = YES;
		self.autocapitalizationType = UITextAutocapitalizationTypeNone;
	}
	return self;
}

- (void)setInputType:(Reflex::System::VirtualKeyboardInputType)type {
	auto prev_keyboardType = self.keyboardType;
	auto prev_secureEntry = self.isSecureTextEntry;

	switch (type) {
	   case Reflex::System::kVirtualKeyboardInputURL:
		   self.keyboardType = UIKeyboardTypeURL;
		   break;

	   case Reflex::System::kVirtualKeyboardInputEmail:
		   self.keyboardType = UIKeyboardTypeEmailAddress;
		   break;

	   case Reflex::System::kVirtualKeyboardInputNumber:
		   self.keyboardType = UIKeyboardTypeNumberPad;
		   break;

	   case Reflex::System::kVirtualKeyboardInputPhoneNumber:
		   self.keyboardType = UIKeyboardTypePhonePad;
		   break;

	   default:
		   self.keyboardType = UIKeyboardTypeDefault;
		   break;
	}

	self.secureTextEntry = type == Reflex::System::kVirtualKeyboardInputPassword;

	if (self.isSecureTextEntry != prev_secureEntry || self.keyboardType != prev_keyboardType) {
	   [self reloadInputViews];
	}
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
	return NO;
}

// Prevent iOS from suggesting other menus like Autofill
- (UITextInputAssistantItem *)inputAssistantItem {
	UITextInputAssistantItem *item = [super inputAssistantItem];
	item.leadingBarButtonGroups = @[];
	item.trailingBarButtonGroups = @[];
	return item;
}

@end

@implementation ReflexTextFieldForKeyboardInput

- (void)deleteBackward {
	// Deleting the first character means that we delete further than the current input, we'll just send a backspace
	if ([self offsetFromPosition:self.beginningOfDocument toPosition:self.selectedTextRange.start] == 0) {
		if (auto window = Reflex::System::iOS::Window::st_self) {
			window->Internal_HandleImeInput(1, {});
		}
	}

	[super deleteBackward];
}

@end


@implementation ReflexViewController

- (void)viewDidLoad {
	[super viewDidLoad];

	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardWillChange:) name:UIKeyboardWillChangeFrameNotification object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];

	self.view.backgroundColor = UIColor.blackColor;
	self.view.contentScaleFactor = UIScreen.mainScreen.scale;

	Reflex::System::iOS::Window::st_viewcontroller = self;

	m_keyboardinputview = [[ReflexTextFieldForKeyboardInput alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
	m_keyboardinputview.delegate = self;
	m_keyboardinputview.inputAssistantItem.leadingBarButtonGroups = @[];
	m_keyboardinputview.inputAssistantItem.trailingBarButtonGroups = @[];
	m_keyboardinputview.autocorrectionType = UITextAutocorrectionTypeNo;
	m_keyboardinputview.layer.zPosition = -1; // backdrop
//	m_keyboardinputview.hidden = YES;

	[self.view addSubview:m_keyboardinputview];
	[self.view.window makeKeyAndVisible];

	AppType::Get()->OpenEditor();
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	REFLEX_USE(Reflex::System)

	switch (iOS::Window::st_self->GetClient()->OnGetScreenOrientation()) {
		case kScreenOrientationDefault:
			return UIInterfaceOrientationMaskAllButUpsideDown;

		case kScreenOrientationPortrait:
			return UIInterfaceOrientationMaskPortrait;

		case kScreenOrientationLandscape:
			return UIInterfaceOrientationMaskLandscape;
	}
}

- (BOOL)destroyOnEnterBackground {
	return NO;
}

- (void)applicationWillEnterForeground:(nonnull NSNotification*)notification {
	self.view.backgroundColor = UIColor.blackColor;
	self.view.contentScaleFactor = UIScreen.mainScreen.scale;

	[[self.view.window viewWithTag:K32("snapshot")] removeFromSuperview];

	AppType::Get()->OpenEditor();
}

- (void)applicationDidEnterBackground:(nonnull NSNotification*)notification {
	// Take a screenshot before we close the editor, and use it as the app thumbnail
	dispatch_async(dispatch_get_main_queue(), ^{
		UIImage* snapshot = [self.view takeSnapshot];
		if (snapshot) {
			auto* imageView = [[UIImageView alloc] initWithImage:snapshot];
			imageView.frame = self.view.bounds;
			imageView.tag = K32("snapshot");
			[self.view.window addSubview:imageView];
		}
		AppType::Get()->CloseEditor();
	});
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
	REFLEX_USE(Reflex::System)

	[super traitCollectionDidChange:previousTraitCollection];

	if (self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle) {
		iOS::globals->m_signals[kNotificationChangeDisplays].Notify();
	}
}

- (void)viewSizeDidChange {
	if (auto window = Reflex::System::iOS::Window::st_self) {
		window->UpdateViewSize();
	}

	[self layoutTextEditorViews];
}

- (void)viewDidLayoutSubviews {
	[super viewDidLayoutSubviews];
	[self viewSizeDidChange];
}

- (UIEdgeInsets)viewInsets {
	UIEdgeInsets insets = self.view.window.safeAreaInsets;
	if (m_keyboardShown) {
		insets.bottom = m_keyboardRect.size.height;
	}
	return insets;
}

- (void)pressesBegan:(nonnull NSSet<UIPress *> *)presses withEvent:(nullable UIPressesEvent *)event {
	// Let the native controls handle this one
	if (![self isUsingNativeTextInputBox]) {
		// Tells this object when a physical button is first pressed.
		if (auto window = Reflex::System::iOS::Window::st_self) {
			if (window->Internal_ProcessKeyDown(event)) return;
		}
	}

	[super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(nonnull NSSet<UIPress *> *)presses withEvent:(nullable UIPressesEvent *)event {
	if (![self isUsingNativeTextInputBox]) {
		// Tells the object when a button is released.
		if (auto window = Reflex::System::iOS::Window::st_self) {
			window->Internal_ProcessKeyUp(event);
		}
	}

	[super pressesEnded:presses withEvent:event];
}

- (void)pressesCancelled:(nonnull NSSet<UIPress *> *)presses withEvent:(nullable UIPressesEvent *)event {
	if (![self isUsingNativeTextInputBox]) {
		// Tells this object when a system event (such as a low-memory warning) cancels a press event.
		if (auto window = Reflex::System::iOS::Window::st_self) {
			window->Internal_ProcessKeyUp(event);
		}
	}

	[super pressesCancelled:presses withEvent:event];
}

@end
