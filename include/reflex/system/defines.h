#pragma once

#include "[require].h"




//
//Secondary API

namespace Reflex::System
{

	enum Platform : UInt8
	{
		kPlatformWindows,
		kPlatformMacOS,
		kPlatformLinux,

		kPlatformAndroid,
		kPlatformIOS,

		kPlatformWebAssembly,

		kNumPlatform
	};
	
	enum EnvironmentType : UInt8
	{
		kEnvironmentTypeLibrary,
		kEnvironmentTypeConsoleApp,
		kEnvironmentTypeDesktopApp,
		kEnvironmentTypeMobileApp,
		kEnvironmentTypeAudioPlugin,

		kNumEnvironmentType
	};

	enum Notification : UInt8
	{
		kNotificationClock,
		kNotificationChangeDisplays,
		kNotificationChangeDevices,

		kNumNotification,
	};

	enum Priority : UInt8
	{
		kPriorityBackground,
		kPriorityNormal,
		kPriorityHigh,

		kNumPriority,
	};

	enum ImageFormat : UInt8
	{
		kImageFormatRGB,
		kImageFormatBGR,
		kImageFormatRGBA,
		kImageFormatBGRA,
		kImageFormatLuminance,

		kNumImageFormat,
	};

	enum KeyCode : UInt8
	{
		kKeyCodeNull,

		kKeyCodeF1, kKeyCodeF2, kKeyCodeF3, kKeyCodeF4, kKeyCodeF5, kKeyCodeF6, kKeyCodeF7, kKeyCodeF8, kKeyCodeF9, kKeyCodeF10, kKeyCodeF11, kKeyCodeF12,

		kKeyCodeTab,
		kKeyCodeEnter,
		kKeyCodeEscape,
		kKeyCodeSpace,
		kKeyCodeBackspace,

		kKeyCodeInsert,
		kKeyCodeDelete,
		kKeyCodeHome,
		kKeyCodeEnd,
		kKeyCodePageUp,
		kKeyCodePageDown,

		kKeyCodeUp,
		kKeyCodeDown,
		kKeyCodeLeft,
		kKeyCodeRight,

		kKeyCodeNumericDivide,
		kKeyCodeNumericMultiply,
		kKeyCodeNumericMinus,
		kKeyCodeNumericPlus,

		kKeyCode1, kKeyCode2, kKeyCode3, kKeyCode4, kKeyCode5, kKeyCode6, kKeyCode7, kKeyCode8, kKeyCode9, kKeyCode0,
	
		kKeyCodeMinus, kKeyCodePlus, kKeyCodeSlash,

		kKeyCodeA, kKeyCodeB, kKeyCodeC, kKeyCodeD, kKeyCodeE, kKeyCodeF, kKeyCodeG, kKeyCodeH, kKeyCodeI, kKeyCodeJ, kKeyCodeK, kKeyCodeL, kKeyCodeM, kKeyCodeN, kKeyCodeO, kKeyCodeP, kKeyCodeQ, kKeyCodeR, kKeyCodeS, kKeyCodeT, kKeyCodeU, kKeyCodeV, kKeyCodeW, kKeyCodeX, kKeyCodeY, kKeyCodeZ,

		kKeyCodeBracketOpen, kKeyCodeBracketClose,

		kNumKeyCode,
	};

	enum ModifierKeys : UInt8
	{
		kModifierKeyShift = 1,
		kModifierKeyCtrl = 2,
		kModifierKeyAlt = 4,
		kModifierKeySystem = 8,
	};

	enum Path : UInt8
	{
		kPathTemp,
		kPathDesktop,
		kPathApplicationData,
		kPathUserData,
		kPathUserDocuments,

		kNumPath,
	};

	extern const Platform kPlatform;

	extern const EnvironmentType kEnvironmentType;

	constexpr WChar kPathDelimiter = '/';	//All paths must use this character (System takes care of conversion to/from the platform-specific delimiter)

	extern const UInt8 kBPP[kNumImageFormat];

}
