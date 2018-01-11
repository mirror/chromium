// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_ENVIRONMENT_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_ENVIRONMENT_H_

#include "services/identity/public/cpp/identity_manager.h"

namespace identity {

constexpr char kDefaultGaiaId[] = "default_gaia";
constexpr char kDefaultEmail[] = "default_email";
constexpr char kDefaultRefreshToken[] = "default_token";

class IdentityTestEnvironmentInternal;

// Class that creates an IdentityManager for use in testing contexts and
// provides facilities for driving that IdentityManager. The IdentityManager
// instance is brought up in an environment where the primary account is
// available (signed in with a refresh token) with the default values listed
// above.
class IdentityTestEnvironment : public IdentityManager::DiagnosticsObserver {
 public:
  IdentityTestEnvironment();
  ~IdentityTestEnvironment() override;

  // The IdentityManager instance created and owned by this instance.
  IdentityManager* identity_manager();

  // When this is set, access token requests will be automatically granted with
  // an access token value of "access_token".
  void AutomaticallyGrantAccessTokenRequests(bool grant);

  // Runs a nested runloop until an access token request is observed.
  void WaitForAccessTokenRequest();

  // Issues |error| to any pending access token request.
  void IssueErrorForAllPendingAccessTokenRequests(
      const GoogleServiceAuthError& error);

 private:
  // IdentityManager::DiagnosticsObserver:
  void OnAccessTokenRequested(
      const std::string& account_id,
      const std::string& consumer_id,
      const OAuth2TokenService::ScopeSet& scopes) override;

  std::unique_ptr<IdentityTestEnvironmentInternal> internals_;
  base::OnceClosure on_access_token_requested_callback_;

  DISALLOW_COPY_AND_ASSIGN(IdentityTestEnvironment);
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_ENVIRONMENT_H_
