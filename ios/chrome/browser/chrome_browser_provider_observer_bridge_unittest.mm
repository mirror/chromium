// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/chrome_browser_provider_observer_bridge.h"

#include <memory>

#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestChromeBrowserProviderObserver
    : NSObject<ChromeBrowserProviderObserver>
@property(nonatomic) BOOL onChromeIdentityServiceDidChangeCalled;
@property(nonatomic) BOOL onChromeBrowserProviderWillBeDestroyedCalled;
@property(nonatomic) ios::ChromeIdentityService* identityService;
@property(nonatomic, readonly)
    ios::ChromeBrowserProvider::Observer* observerBridge;
@end

@implementation TestChromeBrowserProviderObserver {
  std::unique_ptr<ios::ChromeBrowserProvider::Observer> observer_bridge_;
}

@synthesize onChromeIdentityServiceDidChangeCalled =
    _onChromeIdentityServiceDidChangeCalled;
@synthesize onChromeBrowserProviderWillBeDestroyedCalled =
    _onChromeBrowserProviderWillBeDestroyedCalled;
@synthesize identityService = _identityService;

- (instancetype)init {
  if (self == [super init]) {
    observer_bridge_.reset(new ChromeBrowserProviderObserverBridge(self));
  }
  return self;
}

- (ios::ChromeBrowserProvider::Observer*)observerBridge {
  return observer_bridge_.get();
}

#pragma mark - ios::ChromeBrowserProvider::Observer

- (void)onChromeIdentityServiceDidChange:
    (ios::ChromeIdentityService*)identityService {
  _onChromeIdentityServiceDidChangeCalled = YES;
  _identityService = identityService;
}

- (void)onChromeBrowserProviderWillBeDestroyed {
  _onChromeBrowserProviderWillBeDestroyedCalled = YES;
}

@end

#pragma mark - ChromeBrowserProviderObserverBridgeTest

class ChromeBrowserProviderObserverBridgeTest : public PlatformTest {
 protected:
  ChromeBrowserProviderObserverBridgeTest()
      : test_observer_([[TestChromeBrowserProviderObserver alloc] init]) {}

  ios::ChromeBrowserProvider::Observer* GetObserverBridge() {
    return [test_observer_ observerBridge];
  }

  TestChromeBrowserProviderObserver* GetTestObserver() {
    return test_observer_;
  }

 private:
  TestChromeBrowserProviderObserver* test_observer_;
};

// Tests that |onChromeIdentityServiceDidChange| is forwarded.
TEST_F(ChromeBrowserProviderObserverBridgeTest,
       onChromeIdentityServiceDidChange) {
  ios::ChromeIdentityService* identityService =
      new ios::ChromeIdentityService();
  ASSERT_FALSE(GetTestObserver().onChromeIdentityServiceDidChangeCalled);
  GetObserverBridge()->OnChromeIdentityServiceDidChange(identityService);
  EXPECT_TRUE(GetTestObserver().onChromeIdentityServiceDidChangeCalled);
  EXPECT_FALSE(GetTestObserver().onChromeBrowserProviderWillBeDestroyedCalled);
  EXPECT_EQ(identityService, GetTestObserver().identityService);
  delete (identityService);
}

// Tests that |onChromeBrowserProviderWillBeDestroyed| is forwarded.
TEST_F(ChromeBrowserProviderObserverBridgeTest,
       onChromeBrowserProviderWillBeDestroyed) {
  ASSERT_FALSE(GetTestObserver().onChromeBrowserProviderWillBeDestroyedCalled);
  GetObserverBridge()->OnChromeBrowserProviderWillBeDestroyed();
  EXPECT_FALSE(GetTestObserver().onChromeIdentityServiceDidChangeCalled);
  EXPECT_TRUE(GetTestObserver().onChromeBrowserProviderWillBeDestroyedCalled);
}
