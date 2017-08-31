// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/signin_earlgrey_utils.h"

#import <EarlGrey/EarlGrey.h>

#include "base/strings/sys_string_conversions.h"
#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::PrimarySignInButton;
using chrome_test_util::SecondarySignInButton;

@implementation SigninEarlGreyUtils

+ (void)checkSigninPromoVisibleWithMode:(SigninPromoViewMode)mode {
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(
                                   grey_accessibilityID(kSigninPromoViewId),
                                   grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(PrimarySignInButton(),
                                          grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_notNil()];
  switch (mode) {
    case SigninPromoViewModeColdState:
      [[EarlGrey
          selectElementWithMatcher:grey_allOf(SecondarySignInButton(),
                                              grey_sufficientlyVisible(), nil)]
          assertWithMatcher:grey_nil()];
      break;
    case SigninPromoViewModeWarmState:
      [[EarlGrey
          selectElementWithMatcher:grey_allOf(SecondarySignInButton(),
                                              grey_sufficientlyVisible(), nil)]
          assertWithMatcher:grey_notNil()];
      break;
  }
}

+ (void)checkSigninPromoNotVisible {
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(
                                   grey_accessibilityID(kSigninPromoViewId),
                                   grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_nil()];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(PrimarySignInButton(),
                                          grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_nil()];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(SecondarySignInButton(),
                                          grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_nil()];
}

+ (ChromeIdentity*)fakeIdentity1 {
  return [FakeChromeIdentity identityWithEmail:@"foo@gmail.com"
                                        gaiaID:@"fooID"
                                          name:@"Fake Foo"];
}

+ (ChromeIdentity*)fakeIdentity2 {
  return [FakeChromeIdentity identityWithEmail:@"bar@gmail.com"
                                        gaiaID:@"barID"
                                          name:@"Fake Bar"];
}

+ (ChromeIdentity*)fakeIdentity3 {
  return [FakeChromeIdentity identityWithEmail:@"managed@foo.com"
                                        gaiaID:@"managedID"
                                          name:@"Fake Managed"];
}

// Asserts that |identity| is actually signed in to the active profile.
+ (void)assertActiveProfileWithIdentity:(ChromeIdentity*)identity {
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];

  ios::ChromeBrowserState* browser_state =
      chrome_test_util::GetOriginalBrowserState();
  AccountInfo info =
      ios::SigninManagerFactory::GetForBrowserState(browser_state)
          ->GetAuthenticatedAccountInfo();

  GREYAssertEqual(base::SysNSStringToUTF8(identity.gaiaID), info.gaia,
                  @"Unexpected Gaia ID of the signed in user [expected = "
                  @"\"%@\", actual = \"%s\"]",
                  identity.gaiaID, info.gaia.c_str());
}

@end
