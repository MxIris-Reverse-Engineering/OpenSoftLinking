#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <CoreFoundation/CoreFoundation.h>

OPEN_SOFT_LINK_FRAMEWORK(CoreFoundation)
OPEN_SOFT_LINK(CoreFoundation, CFStringGetLength, CFIndex,
               (CFStringRef theString),
               (theString))

@interface OSLMacroFunctionTests : XCTestCase
@end

@implementation OSLMacroFunctionTests

- (void)test_FUNCTION_invokesReal {
    CFStringRef s = CFSTR("hello");
    CFIndex len = CFStringGetLength_soft(s);
    XCTAssertEqual(len, 5);
}

@end
