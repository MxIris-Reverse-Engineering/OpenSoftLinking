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

@end
