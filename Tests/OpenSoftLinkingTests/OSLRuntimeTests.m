//
//  OSLRuntimeTests.m
//  OpenSoftLinkingTests
//

#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <dlfcn.h>
#include <string.h>
#include <stdlib.h>

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

- (void)test_dlopen_allFail_errorMessageSet {
    const char *const paths[] = {
        "/does/not/exist/A",
        "/does/not/exist/B",
        NULL
    };
    char *err = NULL;
    void *handle = _osl_dlopen(paths, &err);
    XCTAssertTrue(handle == NULL);
    XCTAssertTrue(err != NULL, "Expected an aggregated error string");
    XCTAssertTrue(strstr(err, "\n") != NULL, "Expected '\\n' separator");
    free(err);
}

- (void)test_dlopen_allFail_errorMessageIsFreeable {
    const char *const paths[] = { "/does/not/exist/X", NULL };
    char *err = NULL;
    void *handle = _osl_dlopen(paths, &err);
    XCTAssertTrue(handle == NULL);
    XCTAssertTrue(err != NULL);
    free(err);  /* should not crash */
}

- (void)test_dlopen_success_doesNotTouchErrorMessage {
    const char *const paths[] = {
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        NULL
    };
    char *err = (char *)0xDEADBEEF;
    void *handle = _osl_dlopen(paths, &err);
    XCTAssertTrue(handle != NULL);
    XCTAssertEqual((void *)err, (void *)0xDEADBEEF, "err must be untouched on success");
}

- (void)test_dlopen_emptyPathList_withErrorMessage_returnsEmptyString {
    const char *const paths[] = { NULL };
    char *err = NULL;
    void *handle = _osl_dlopen(paths, &err);
    XCTAssertTrue(handle == NULL);
    XCTAssertTrue(err != NULL, "Expected calloc-allocated empty string");
    XCTAssertEqual(err[0], '\0', "Expected empty string for empty path list");
    free(err);
}

@end
