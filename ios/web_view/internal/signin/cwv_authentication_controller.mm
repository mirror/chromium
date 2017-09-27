// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/signin/cwv_authentication_controller_internal.h"

#include "base/strings/sys_string_conversions.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_delegate.h"
#include "ios/web_view/internal/signin/web_view_account_tracker_service_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_profile_oauth2_token_service_ios_provider_impl.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#import "ios/web_view/public/cwv_authentication_controller_delegate.h"
#import "ios/web_view/public/cwv_identity.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CWVAuthenticationController {
  ios_web_view::WebViewBrowserState* _browserState;
}

@synthesize delegate = _delegate;

- (instancetype)initWithBrowserState:
    (ios_web_view::WebViewBrowserState*)browserState {
  self = [super init];
  if (self) {
    _browserState = browserState;
  }
  return self;
}

#pragma mark - Public Methods

- (void)setDelegate:(id<CWVAuthenticationControllerDelegate>)delegate {
  _delegate = delegate;

  ProfileOAuth2TokenService* tokenService =
      ios_web_view::WebViewOAuth2TokenServiceFactory::GetForBrowserState(
          _browserState);
  ProfileOAuth2TokenServiceIOSDelegate* tokenServiceDelegate =
      static_cast<ProfileOAuth2TokenServiceIOSDelegate*>(
          tokenService->GetDelegate());
  WebViewProfileOAuth2TokenServiceIOSProviderImpl* tokenProvider =
      static_cast<WebViewProfileOAuth2TokenServiceIOSProviderImpl*>(
          tokenServiceDelegate->GetProvider());
  tokenProvider->set_authentication_controller(self);

  std::string authenticatedAccountId =
      ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
          _browserState)
          ->GetAuthenticatedAccountId();
  if (!authenticatedAccountId.empty()) {
    tokenService->LoadCredentials(authenticatedAccountId);
  }
}

- (CWVIdentity*)currentIdentity {
  std::string authenticatedAccountId =
      ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
          _browserState)
          ->GetAuthenticatedAccountId();

  AccountTrackerService* accountTracker =
      ios_web_view::WebViewAccountTrackerServiceFactory::GetForBrowserState(
          _browserState);
  AccountInfo accountInfo =
      accountTracker->GetAccountInfo(authenticatedAccountId);
  std::string authenticatedGaiaId = accountInfo.gaia;
  if (authenticatedGaiaId.empty()) {
    return nil;
  }
  CWVIdentity* identity = [[CWVIdentity alloc] init];
  identity.gaiaID = base::SysUTF8ToNSString(authenticatedGaiaId);
  identity.userEmail = base::SysUTF8ToNSString(accountInfo.email);
  identity.userFullName = base::SysUTF8ToNSString(accountInfo.full_name);
  return identity;
}

- (void)signInWithIdentity:(CWVIdentity*)identity {
  AccountTrackerService* accountTracker =
      ios_web_view::WebViewAccountTrackerServiceFactory::GetForBrowserState(
          _browserState);
  AccountInfo info;
  info.gaia = base::SysNSStringToUTF8([identity gaiaID]);
  info.email = base::SysNSStringToUTF8([identity userEmail]);
  std::string newAuthenticatedAccountId = accountTracker->SeedAccountInfo(info);
  std::string oldAuthenticatedAccountId =
      ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(
          _browserState)
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
  ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(_browserState)
      ->OnExternalSigninCompleted(newAuthenticatedUsername);

  // Reload all credentials to match the desktop model. Exclude all the
  // accounts ids that are the primary account ids on other profiles.
  OAuth2TokenService* tokenService =
      ios_web_view::WebViewOAuth2TokenServiceFactory::GetForBrowserState(
          _browserState);
  ProfileOAuth2TokenServiceIOSDelegate* tokenServiceDelegate =
      static_cast<ProfileOAuth2TokenServiceIOSDelegate*>(
          tokenService->GetDelegate());
  tokenServiceDelegate->ReloadCredentials(newAuthenticatedAccountId);
}

- (void)signOut {
  ios_web_view::WebViewSigninManagerFactory::GetForBrowserState(_browserState)
      ->SignOut(signin_metrics::ProfileSignout::USER_CLICKED_SIGNOUT_SETTINGS,
                signin_metrics::SignoutDelete::IGNORE_METRIC);
}

@end
