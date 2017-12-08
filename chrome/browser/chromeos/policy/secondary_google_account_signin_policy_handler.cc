// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/secondary_google_account_signin_policy_handler.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/signin/core/browser/signin_pref_names.h"

namespace policy {

SecondaryGoogleAccountSigninPolicyHandler::
    SecondaryGoogleAccountSigninPolicyHandler()
    : TypeCheckingPolicyHandler(key::kSecondaryGoogleAccountSigninAllowed,
                                base::Value::Type::BOOLEAN) {}

SecondaryGoogleAccountSigninPolicyHandler::
    ~SecondaryGoogleAccountSigninPolicyHandler() {}

void SecondaryGoogleAccountSigninPolicyHandler::ApplyPolicySettings(
    const PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value =
      policies.GetValue(key::kSecondaryGoogleAccountSigninAllowed);

  if (!value) {
    return;
  }

  const bool is_secondary_signin_allowed = value->GetBool();

  // The condition for the preference is negation of the policy.
  // Mirror consistency will be enabled when secondary sign-in is disallowed.
  prefs->SetBoolean(prefs::kAccountConsistencyMirrorRequired,
                    !is_secondary_signin_allowed);
}

}  // namespace policy
