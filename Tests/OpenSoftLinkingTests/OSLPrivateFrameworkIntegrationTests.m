//
//  OSLPrivateFrameworkIntegrationTests.m
//  OpenSoftLinkingTests
//
//  Integration smoke tests that exercise the PRIVATE_FRAMEWORK family of
//  macros against real Apple private frameworks. These are not intended as
//  regression tests for specific framework APIs (Apple may rename these
//  classes between macOS versions); they verify that the macro suite can
//  successfully load and resolve symbols from /System/Library/PrivateFrameworks.
//

#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <objc/runtime.h>

/* CoreUI (artwork/catalog rendering) — present on every modern macOS. */
OPEN_SOFT_LINK_PRIVATE_FRAMEWORK(CoreUI)
OPEN_SOFT_LINK_CLASS(CoreUI, CUICatalog)

/* IconServices (icon rendering service) — present on every modern macOS. */
OPEN_SOFT_LINK_PRIVATE_FRAMEWORK(IconServices)
OPEN_SOFT_LINK_CLASS(IconServices, ISUtilities)

/* AOSKit (Apple Online Services) — present on macOS desktop. */
OPEN_SOFT_LINK_PRIVATE_FRAMEWORK(AOSKit)
OPEN_SOFT_LINK_CLASS(AOSKit, AOSUtilities)

/* A second class from the same private framework — verifies that multiple
 * CLASS instantiations coexist with a single PRIVATE_FRAMEWORK loader. */
OPEN_SOFT_LINK_CLASS(AOSKit, AOSAccountCache)

/* An intentionally-absent private framework, used to verify that the OPTIONAL
 * variant returns NULL rather than aborting. */
OPEN_SOFT_LINK_PRIVATE_FRAMEWORK_OPTIONAL(OpenSoftLinkingDoesNotExist_XYZZY)

@interface OSLPrivateFrameworkIntegrationTests : XCTestCase
@end

@implementation OSLPrivateFrameworkIntegrationTests

- (void)test_PRIVATE_FRAMEWORK_CoreUI_loadsAndResolvesCUICatalog {
    XCTAssertTrue(CoreUILibrary() != NULL, "Expected CoreUI private framework to load");
    Class catalog = getCUICatalogClass();
    XCTAssertNotNil(catalog);
    XCTAssertEqualObjects(NSStringFromClass(catalog), @"CUICatalog");
}

- (void)test_PRIVATE_FRAMEWORK_IconServices_loadsAndResolvesISUtilities {
    XCTAssertTrue(IconServicesLibrary() != NULL, "Expected IconServices to load");
    Class util = getISUtilitiesClass();
    XCTAssertNotNil(util);
    XCTAssertEqualObjects(NSStringFromClass(util), @"ISUtilities");
}

- (void)test_PRIVATE_FRAMEWORK_AOSKit_loadsTwoClassesFromOneFramework {
    XCTAssertTrue(AOSKitLibrary() != NULL, "Expected AOSKit to load");
    Class utilities = getAOSUtilitiesClass();
    Class cache = getAOSAccountCacheClass();
    XCTAssertNotNil(utilities);
    XCTAssertNotNil(cache);
    XCTAssertNotEqual(utilities, cache, "Distinct classes must be distinct");
}

- (void)test_PRIVATE_FRAMEWORK_loadersAreCached {
    void *a = CoreUILibrary();
    void *b = CoreUILibrary();
    XCTAssertEqual(a, b, "Second call must return the same dyld handle");
}

- (void)test_PRIVATE_FRAMEWORK_OPTIONAL_missingFramework_returnsNull {
    void *handle = OpenSoftLinkingDoesNotExist_XYZZYLibrary();
    XCTAssertTrue(handle == NULL, "OPTIONAL loader must return NULL for missing framework");
}

- (void)test_PRIVATE_FRAMEWORK_dladdr_imagePathContainsPrivateFrameworks {
    Class catalog = getCUICatalogClass();
    XCTAssertNotNil(catalog);
    Dl_info info = {0};
    int ok = dladdr((__bridge const void *)catalog, &info);
    XCTAssertTrue(ok != 0, "dladdr should succeed for a loaded class");
    XCTAssertTrue(strstr(info.dli_fname, "CoreUI") != NULL,
                  "dli_fname should reference CoreUI: %s", info.dli_fname);
}

@end
