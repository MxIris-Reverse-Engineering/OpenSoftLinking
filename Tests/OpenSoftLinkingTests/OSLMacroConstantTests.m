#import <XCTest/XCTest.h>
#import <OpenSoftLinking/OpenSoftLinking.h>
#import <Foundation/Foundation.h>

OPEN_SOFT_LINK_FRAMEWORK(Foundation)
OPEN_SOFT_LINK_CONSTANT(Foundation, NSCocoaErrorDomain, NSErrorDomain)
OPEN_SOFT_LINK_VARIABLE(Foundation, NSProcessInfoThermalStateDidChangeNotification, NSNotificationName)

@interface OSLMacroConstantTests : XCTestCase
@end

@implementation OSLMacroConstantTests

- (void)test_CONSTANT_resolvesCorrectly {
    XCTAssertEqualObjects(getNSCocoaErrorDomain(), NSCocoaErrorDomain);
}

- (void)test_VARIABLE_getterReturnsSameAsDirect {
    XCTAssertEqualObjects(getNSProcessInfoThermalStateDidChangeNotification(),
                          NSProcessInfoThermalStateDidChangeNotification);
}

@end
