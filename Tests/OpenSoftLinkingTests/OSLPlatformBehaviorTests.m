#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <TargetConditionals.h>
#import <dlfcn.h>

@interface OSLPlatformBehaviorTests : XCTestCase
@end

@implementation OSLPlatformBehaviorTests

#if TARGET_OS_MACCATALYST
- (void)test_catalyst_iOSPath_resolvesViaIOSSupport {
    const char *const paths[] = {
        "/System/Library/Frameworks/UIKit.framework/UIKit",
        NULL
    };
    void *handle = _osl_dlopen(paths, NULL);
    XCTAssertTrue(handle != NULL, "UIKit should load on Catalyst");

    void *sym = dlsym(handle, "OBJC_CLASS_$_UIApplication");
    XCTAssertTrue(sym != NULL);
    Dl_info info;
    XCTAssertTrue(dladdr(sym, &info) != 0);
    XCTAssertTrue(strstr(info.dli_fname, "/System/iOSSupport/") != NULL,
                  "Loaded image should live under /System/iOSSupport/");
}
#else
- (void)test_catalyst_iOSPath_resolvesViaIOSSupport {
    XCTSkip(@"Catalyst-only behavior.");
}
#endif

- (void)test_concurrent_dlopen_sameDescriptor_sameHandle {
    const char *const paths[] = {
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        NULL
    };
    const char *const *pathsPtr = paths;
    __block void *first = NULL;
    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t q = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
    for (int i = 0; i < 100; ++i) {
        dispatch_group_async(group, q, ^{
            void *h = _osl_dlopen(pathsPtr, NULL);
            if (!first) first = h;
            XCTAssertEqual(h, first);
        });
    }
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    XCTAssertTrue(first != NULL);
}

/* Indirect PAC verification: call _osl_dlopen via a wrapper function that
 * has its own PC. If PAC stripping works, the call still succeeds. */
static void *loadViaHelper(const char *const *paths) {
    return _osl_dlopen(paths, NULL);
}

- (void)test_caller_propagation_viaHelperFunction {
    const char *const paths[] = {
        "/System/Library/Frameworks/Foundation.framework/Foundation",
        NULL
    };
    XCTAssertTrue(loadViaHelper(paths) != NULL);
}

@end
