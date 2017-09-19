// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/test_utils.h"

#include "chrome/browser/signin/account_fetcher_service_factory.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/fake_account_fetcher_service_builder.h"
#include "chrome/browser/signin/fake_gaia_cookie_manager_service_builder.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"
#include "chrome/browser/signin/fake_signin_manager_builder.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/test_signin_client_builder.h"

void InitializeFakeSigninFactories(TestingProfile::Builder& builder,
                                   int options) {
  if (options & FAKE_ACCOUNT_FETCHER) {
    builder.AddTestingFactory(AccountFetcherServiceFactory::GetInstance(),
                              FakeAccountFetcherServiceBuilder::BuildForTests);
  }
  if (options & FAKE_GAIA_COOKIE_MANAGER) {
    builder.AddTestingFactory(GaiaCookieManagerServiceFactory::GetInstance(),
                              BuildFakeGaiaCookieManagerService);
  }
  if (options & FAKE_SIGNIN_CLIENT) {
    builder.AddTestingFactory(ChromeSigninClientFactory::GetInstance(),
                              signin::BuildTestSigninClient);
  }
  if (options & FAKE_SIGNIN_MANAGER) {
    builder.AddTestingFactory(SigninManagerFactory::GetInstance(),
                              BuildFakeSigninManagerBase);
  }
  if (options & FAKE_TOKEN_SERVICE) {
    builder.AddTestingFactory(ProfileOAuth2TokenServiceFactory::GetInstance(),
                              BuildFakeProfileOAuth2TokenService);
  }
}

std::unique_ptr<TestingProfile> CreateTestingProfileForSignin() {
  TestingProfile::Builder builder;
  InitializeFakeSigninFactories(builder);
  return builder.Build();
}

FakeProfileOAuth2TokenService* GetFakeTokenServiceForProfile(Profile* profile) {
  return static_cast<FakeProfileOAuth2TokenService*>(
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile));
}
FakeSigninManager* GetFakeSigninManagerForProfile(Profile* profile) {
  return static_cast<FakeSigninManager*>(
      SigninManagerFactory::GetForProfile(profile));
}
FakeGaiaCookieManagerService* GetFakeGaiaCookieManagerForProfile(
    Profile* profile) {
  return static_cast<FakeGaiaCookieManagerService*>(
      GaiaCookieManagerServiceFactory::GetForProfile(profile));
}
FakeAccountFetcherService* GetFakeAccountFetcherForProfile(Profile* profile) {
  return static_cast<FakeAccountFetcherService*>(
      AccountFetcherServiceFactory::GetForProfile(profile));
}
