// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_TEST_UTILS_H_
#define CHROME_BROWSER_SIGNIN_TEST_UTILS_H_

#include "chrome/test/base/testing_profile.h"

class FakeAccountFetcherService;
class FakeGaiaCookieManagerService;
class FakeProfileOAuth2TokenService;
class FakeSigninManager;
class TestSigninClient;

// Option to initialize the sign-in services.
enum InitializeFakeSigninFactoriesOption {
  FAKE_ACCOUNT_FETCHER = 1 << 0,
  FAKE_GAIA_COOKIE_MANAGER = 1 << 1,
  FAKE_SIGNIN_MANAGER = 1 << 2,
  FAKE_SIGNIN_CLIENT = 1 << 3,
  FAKE_TOKEN_SERVICE = 1 << 4,
};

// Options to use all fake sign-in services.
const int FAKE_ALL_SIGNIN_SERVICES = 0xFF;

// Initializes the fake sign-in factories.
void InitializeFakeSigninFactories(TestingProfile::Builder& profile_builder,
                                   int options = FAKE_ALL_SIGNIN_SERVICES);

// Returns the fake token service from the profile.
FakeProfileOAuth2TokenService* GetFakeTokenServiceForProfile(Profile* profile);

// Returns the fake signin manager from the profile.
FakeSigninManager* GetFakeSigninManagerForProfile(Profile* profile);

// Returns the fake GAIA cookie manager from the profile.
FakeGaiaCookieManagerService* GetFakeGaiaCookieManagerForProfile(
    Profile* profile);

// Returns the fake account fetcher from the profile.
FakeAccountFetcherService* GetFakeAccountFetcherForProfile(Profile* profile);

#endif  // CHROME_BROWSER_SIGNIN_TEST_UTILS_H_
