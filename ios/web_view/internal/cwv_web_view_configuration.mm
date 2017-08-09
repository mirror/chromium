// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/public/cwv_web_view_configuration.h"
#import "ios/web_view/internal/cwv_web_view_configuration_internal.h"

#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_delegate.h"
#include "ios/web_view/internal/app/application_context.h"
#import "ios/web_view/internal/cwv_preferences_internal.h"
#import "ios/web_view/internal/cwv_user_content_controller_internal.h"
#include "ios/web_view/internal/signin/web_view_account_tracker_service_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/web_view_global_state_util.h"
#import "ios/web_view/public/cwv_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CWVWebViewConfiguration () {
  // The BrowserState for this configuration.
  std::unique_ptr<ios_web_view::WebViewBrowserState> _browserState;
}

// Initializes configuration with the specified browser state mode.
- (instancetype)initWithBrowserState:
    (std::unique_ptr<ios_web_view::WebViewBrowserState>)browserState;
@end

@implementation CWVWebViewConfiguration

@synthesize preferences = _preferences;
@synthesize userContentController = _userContentController;

+ (instancetype)defaultConfiguration {
  static CWVWebViewConfiguration* defaultConfiguration;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    auto browserState =
        base::MakeUnique<ios_web_view::WebViewBrowserState>(false);
    defaultConfiguration =
        [[self alloc] initWithBrowserState:std::move(browserState)];
  });
  return defaultConfiguration;
}

+ (instancetype)incognitoConfiguration {
  static CWVWebViewConfiguration* incognitoConfiguration;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    auto browserState =
        base::MakeUnique<ios_web_view::WebViewBrowserState>(true);
    incognitoConfiguration =
        [[self alloc] initWithBrowserState:std::move(browserState)];
  });
  return incognitoConfiguration;
}

+ (void)initialize {
  if (self != [CWVWebViewConfiguration class]) {
    return;
  }

  ios_web_view::InitializeGlobalState();
}

- (instancetype)initWithBrowserState:
    (std::unique_ptr<ios_web_view::WebViewBrowserState>)browserState {
  self = [super init];
  if (self) {
    _browserState = std::move(browserState);

    _preferences =
        [[CWVPreferences alloc] initWithPrefService:_browserState->GetPrefs()];

    _userContentController =
        [[CWVUserContentController alloc] initWithConfiguration:self];
  }
  return self;
}

#pragma mark - Public Methods

- (BOOL)isPersistent {
  return !_browserState->IsOffTheRecord();
}

- (void)signInWithIdentity:(CWVIdentity*)identity {
  AccountTrackerService* accountTracker =
      ios_web_view::WebViewAccountTrackerServiceFactory::GetForBrowserState(
          _browserState.get());
  AccountInfo info;
  info.gaia = base::SysNSStringToUTF8([identity gaiaID]);
  info.email = base::SysNSStringToUTF8([identity userEmail]);
  std::string newAuthenticatedAccountId = accountTracker->SeedAccountInfo(info);
  std::string oldAuthenticatedAccountId =
      ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
          _browserState.get())
          ->GetAuthenticatedAccountId();
  // |SigninManager::SetAuthenticatedAccountId| simply ignores the call if
  // there is already a signed in user. Check that there is no signed in account
  // or that the new signed in account matches the old one to avoid a mismatch
  // between the old and the new authenticated accounts.
  if (!oldAuthenticatedAccountId.empty())
    CHECK_EQ(newAuthenticatedAccountId, oldAuthenticatedAccountId);

  // Update the SigninManager with the new logged in identity.
  std::string newAuthenticatedUsername =
      accountTracker->GetAccountInfo(newAuthenticatedAccountId).email;
  ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
      _browserState.get())
      ->OnExternalSigninCompleted(newAuthenticatedUsername);

  // Reload all credentials to match the desktop model. Exclude all the
  // accounts ids that are the primary account ids on other profiles.
  OAuth2TokenService* tokenService =
      ios_web_view::WebViewOAuth2TokenServiceFactory::GetForBrowserState(
          _browserState.get());
  ProfileOAuth2TokenServiceIOSDelegate* tokenServiceDelegate =
      static_cast<ProfileOAuth2TokenServiceIOSDelegate*>(
          tokenService->GetDelegate());
  tokenServiceDelegate->ReloadCredentials(newAuthenticatedAccountId);
}

#pragma mark - Private Methods

- (ios_web_view::WebViewBrowserState*)browserState {
  return _browserState.get();
}

@end
