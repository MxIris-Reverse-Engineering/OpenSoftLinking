#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <Foundation/Foundation.h>

OPEN_SOFT_LINK_FRAMEWORK(Foundation)
OPEN_SOFT_LINK_POINTER(Foundation, NSDefaultRunLoopMode, NSRunLoopMode)

@interface OSLMacroPointerTests : XCTestCase
@end

@implementation OSLMacroPointerTests

- (void)test_POINTER_resolvesCorrectly {
    NSRunLoopMode viaSoft = getNSDefaultRunLoopMode();
    XCTAssertEqualObjects(viaSoft, NSDefaultRunLoopMode);
}

@end
