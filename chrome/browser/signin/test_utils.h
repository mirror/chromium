// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_TEST_UTILS_H_
#define CHROME_BROWSER_SIGNIN_TEST_UTILS_H_

#include "chrome/test/base/testing_profile.h"

#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_account_fetcher_service.h"
#include "components/signin/core/browser/fake_gaia_cookie_manager_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"

enum InitializeFakeSigninFactoriesOption {
  FAKE_ACCOUNT_FETCHER = 1 << 0,
  FAKE_GAIA_COOKIE_MANAGER = 1 << 1,
  FAKE_SIGNIN_MANAGER = 1 << 2,
  FAKE_SIGNIN_CLIENT = 1 << 3,
  FAKE_TOKEN_SERVICE = 1 << 4,
};

const int FAKE_ALL_SIGNIN_SERVICES = 0xFF;

// Initializes the fake sign-in factories
void InitializeFakeSigninFactories(TestingProfile::Builder& profile_builder,
                                   int options = FAKE_ALL_SIGNIN_SERVICES);

// Returns a testing profile that has all fake sign-in services initialized.
std::unique_ptr<TestingProfile> CreateTestingProfileForSignin();

FakeProfileOAuth2TokenService* GetFakeTokenServiceForProfile(Profile* profile);
FakeSigninManager* GetFakeSigninManagerForProfile(Profile* profile);
FakeGaiaCookieManagerService* GetFakeGaiaCookieManagerForProfile(
    Profile* profile);
FakeAccountFetcherService* GetFakeAccountFetcherForProfile(Profile* profile);

#endif  // CHROME_BROWSER_SIGNIN_TEST_UTILS_H_
