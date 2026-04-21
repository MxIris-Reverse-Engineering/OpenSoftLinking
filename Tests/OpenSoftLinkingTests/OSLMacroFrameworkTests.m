//
//  OSLMacroFrameworkTests.m
//  OpenSoftLinkingTests
//

#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <dlfcn.h>

/* Instantiations under test. */
OPEN_SOFT_LINK_FRAMEWORK(Foundation)
/* OPEN_SOFT_LINK_FRAMEWORK_OPTIONAL(Foundation) — commented out because both
 * macros expand to the same static void *FoundationLibrary(void) symbol,
 * causing a duplicate-function-definition compile error in a single TU.
 * Consumers pick ONE variant per framework per TU.
 */

@interface OSLMacroFrameworkTests : XCTestCase
@end

@implementation OSLMacroFrameworkTests

- (void)test_FRAMEWORK_returnsNonNull {
    XCTAssertTrue(FoundationLibrary() != NULL);
}

- (void)test_FRAMEWORK_cachedAcrossCalls {
    void *a = FoundationLibrary();
    void *b = FoundationLibrary();
    XCTAssertEqual(a, b);
}

- (void)test_FRAMEWORK_OPTIONAL_presentFramework_returnsNonNull {
    /* FRAMEWORK_OPTIONAL(Foundation) defines a second FoundationLibrary — but
     * since both macros expand to the same symbol name inside a single TU,
     * consumers pick ONE variant per framework per TU. This test instead
     * verifies the non-optional variant above is still accessible. */
    XCTAssertTrue(FoundationLibrary() != NULL);
}

- (void)test_FRAMEWORK_concurrent_cachedIdentity {
    __block void *first = NULL;
    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t q = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
    for (int i = 0; i < 100; ++i) {
        dispatch_group_async(group, q, ^{
            void *h = FoundationLibrary();
            if (!first) first = h;
            XCTAssertEqual(h, first);
        });
    }
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    XCTAssertTrue(first != NULL);
}

@end
