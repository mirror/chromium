// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/secondary_google_account_signin_policy_handler.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/strings/grit/components_strings.h"

namespace policy {

SecondaryGoogleAccountSigninPolicyHandler::
    SecondaryGoogleAccountSigninPolicyHandler() {}

SecondaryGoogleAccountSigninPolicyHandler::
    ~SecondaryGoogleAccountSigninPolicyHandler() {}

bool SecondaryGoogleAccountSigninPolicyHandler::CheckPolicySettings(
    const PolicyMap& policies,
    PolicyErrorMap* errors) {
  const base::Value* value =
      policies.GetValue(key::kSecondaryGoogleAccountSigninEnabled);

  if (!value->is_bool()) {
    errors->AddError(key::kSecondaryGoogleAccountSigninEnabled,
                     IDS_POLICY_TYPE_ERROR,
                     base::Value::GetTypeName(base::Value::Type::BOOLEAN));
    return false;
  }

  return true;
}
void SecondaryGoogleAccountSigninPolicyHandler::ApplyPolicySettings(
    const PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value =
      policies.GetValue(key::kSecondaryGoogleAccountSigninEnabled);
  const bool policy_val = value->GetBool();

  // The condition for the preference is negation of the policy.
  // Secondary signin will be enabled when mirror consistency is NOT enabled.
  prefs->SetBoolean(prefs::kAccountConsistencyMirrorRequired, !policy_val);
}

}  // namespace policy
