// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_UTILS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_UTILS_H_

#include <string>

#include "build/build_config.h"

class FakeSigninManagerBase;
class FakeSigninManager;
class ProfileOAuth2TokenService;

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

// Test-related utilities that don't fit in either IdentityTestEnvironment or
// IdentityManager itself. In general, strongly prefer using
// IdentityTestEnvironment to using functions in this file directly.
namespace identity {

class IdentityManager;

// Makes the primary account available with the given values. On non-ChromeOS,
// results in the firing of the IdentityManager and SigninManager callbacks for
// signin success. Blocks until the primary account is available.
// NOTE: Using this method directly is discouraged, but sometimes necessary
// during conversion. Use IdentityTestEnvironment if possible (and
// IdentityTestEnvironment::MakePrimaryAccountAvailable()). This method should
// be used directly only if the production code is using IdentityManager, but it
// is not yet feasible to convert the test code to use IdentityTestEnvironment.
// Any such usage should only be temporary, i.e., should be followed as quickly
// as possible by conversion of the test to use IdentityTestEnvironment.
void MakePrimaryAccountAvailable(SigninManagerForTest* signin_manager,
                                 ProfileOAuth2TokenService* token_service,
                                 IdentityManager* identity_manager,
                                 const std::string& refresh_token,
                                 const std::string& gaia_id,
                                 const std::string& username);

// Clears the primary account. On non-ChromeOS, results in the firing of the
// IdentityManager and SigninManager callbacks for signout. Blocks until the
// primary account is cleared.
// NOTE: The caveats about direct usage given on MakePrimaryAccountAvailable()
// apply here as well.
void ClearPrimaryAccount(SigninManagerForTest* signin_manager,
                         IdentityManager* identity_manager);

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_UTILS_H_
