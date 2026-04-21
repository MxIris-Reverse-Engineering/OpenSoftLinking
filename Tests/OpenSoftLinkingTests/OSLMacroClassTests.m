#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>

OPEN_SOFT_LINK_FRAMEWORK(Foundation)
OPEN_SOFT_LINK_CLASS(Foundation, NSObject)

@interface OSLMacroClassTests : XCTestCase
@end

@implementation OSLMacroClassTests

- (void)test_CLASS_resolvesToRealClass {
    XCTAssertEqualObjects(getNSObjectClass(), [NSObject class]);
}

- (void)test_CLASS_cachedAcrossCalls {
    Class a = getNSObjectClass();
    Class b = getNSObjectClass();
    XCTAssertEqual(a, b);
}

@end
