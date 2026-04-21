//
//  OSLRuntimeTests.m
//  OpenSoftLinkingTests
//

#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <dlfcn.h>

@interface OSLRuntimeTests : XCTestCase
@end

@implementation OSLRuntimeTests

- (void)test_dlopen_primaryPath_returnsHandle {
    const char *const paths[] = {
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        NULL
    };
    void *handle = _osl_dlopen(paths, NULL);
    XCTAssertTrue(handle != NULL, "Expected Foundation.framework to load");
}

- (void)test_dlopen_fallbackUsed_whenPrimaryMissing {
    const char *const paths[] = {
        "/does/not/exist/Foo.framework/Foo",
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        NULL
    };
    void *handle = _osl_dlopen(paths, NULL);
    XCTAssertTrue(handle != NULL, "Fallback Foundation path should succeed");
}

- (void)test_dlopen_allFail_returnsNull_noError {
    const char *const paths[] = {
        "/does/not/exist/A",
        "/does/not/exist/B",
        NULL
    };
    void *handle = _osl_dlopen(paths, NULL);
    XCTAssertTrue(handle == NULL);
}

- (void)test_dlopen_emptyPathList_returnsNull {
    const char *const paths[] = { NULL };
    void *handle = _osl_dlopen(paths, NULL);
    XCTAssertTrue(handle == NULL);
}

- (void)test_dlopen_handle_isUsableWithDlsym {
    const char *const paths[] = {
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        NULL
    };
    void *handle = _osl_dlopen(paths, NULL);
    XCTAssertTrue(handle != NULL);
    void *sym = dlsym(handle, "OBJC_CLASS_$_NSObject");
    XCTAssertTrue(sym != NULL, "Expected NSObject symbol via dlsym");
}

@end
