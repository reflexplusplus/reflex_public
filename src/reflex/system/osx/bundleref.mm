#include "sdk.h"




REFLEX_NS(Reflex::System::OSX)

void * CreateBundleRef(const WString & filename)
{
	Array <UInt8> utf8;

	EncodeUTF8(filename, utf8);

	if (CFURLRef url = CFURLCreateFromFileSystemRepresentation (nullptr, utf8.GetData(), (CFIndex) utf8.GetSize(), true))
	{
		CFBundleRef bundleRef = CFBundleCreate(kCFAllocatorDefault, url);

		CFRelease (url);

		if (bundleRef != nullptr)
		{
			CFErrorRef error = nullptr;

			if (CFBundleLoadExecutableAndReturnError (bundleRef, &error))
			{
				return bundleRef;
			}

			if (error != nullptr)
			{
				//if (CFStringRef failureMessage = CFErrorCopyFailureReason (error))
				//{
				//	DBG (String::fromCFString (failureMessage));
				//	CFRelease (failureMessage);
				//}

				CFRelease(error);
			}

			CFRelease(bundleRef);

			bundleRef = nullptr;
		}
	}
}

void ReleaseBundleRef(void * bundleref)
{
	if (bundleref)
	{
		if (CFBundleIsExecutableLoaded((CFBundleRef)bundleref)) CFBundleUnloadExecutable((CFBundleRef)bundleref);

		CFRelease(bundleref);
	}
}

REFLEX_END;
