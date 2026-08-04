#import <UIKit/UIKit.h>
#include "../common/apple_utils.hpp"

// FIXME: [Florian] put at a better place in common with what's in dialog.mm
#define SECURITY_MODE 0 // iOS
// #define SECURITY_MODE NSURLBookmarkResolutionWithSecurityScope // macOS

REFLEX_BEGIN_INTERNAL(Reflex::System::iOS)

struct ExternalResourceRef : System::ExternalResourceRef {
    explicit ExternalResourceRef(const ArrayView<UInt8>& token)
        : blob(token)
    {}

    Array<UInt8> GetPersistentToken() override {
        return Reinterpret<Array<UInt8>>(blob);
    }

    TRef<FileHandle> Open(FileHandle::Mode mode) override {
		if (!resolvedPath) resolvedPath = retrieveBookmark();
		if (!resolvedPath) return &System::FileHandle::null;

		const char* systemFile = [retrieveBookmark() fileSystemRepresentation];
		WString systemFileW = Common::DecodeUTF8({ Reinterpret<UInt8>(systemFile), RawStringLength(systemFile) });

		return Reflex::System::FileHandle::Create(systemFileW, mode, false);
    }

private:
    Array<UInt8> blob;
	NSURL* resolvedPath = nil;

	NSURL* retrieveBookmark() {
		BOOL bookmarkIsStale = NO;
		NSError* error = nil;
		NSData* bookmarkData = [NSData dataWithBytes:blob.GetData() length:blob.GetSize()];

		// Resolve the bookmark to get the NSURL
		NSURL *url = [NSURL URLByResolvingBookmarkData:bookmarkData
											   options:SECURITY_MODE
										 relativeToURL:nil
								   bookmarkDataIsStale:&bookmarkIsStale
												 error:&error];

		if (error || bookmarkIsStale) {
			// TODO: [Florian] See here what to do in this case https://stackoverflow.com/questions/52524827/withsecurityscope-not-available-in-nsurl-bookmarkcreationoptions
			DEV_LOG("Error resolving bookmark", ToCStringView(error.localizedDescription));
			return nil;
		}

		return url;
	}
};

REFLEX_END_INTERNAL

Reflex::TRef<Reflex::System::ExternalResourceRef> Reflex::System::ExternalResourceRef::Locate(const ArrayView<UInt8>& token) {
    return REFLEX_CREATE(iOS::ExternalResourceRef, token);
}
