// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/policy/secondary_google_account_signin_policy_handler.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class SecondaryGoogleAccountSigninPolicyHandlerTest : public testing::Test {
 protected:
  void SetPolicy(std::unique_ptr<base::Value> value) {
    policies_.Set(key::kSecondaryGoogleAccountSigninEnabled,
                  POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                  POLICY_SOURCE_CLOUD, std::move(value), nullptr);
  }

  bool CheckPolicy(std::unique_ptr<base::Value> value) {
    SetPolicy(std::move(value));
    return handler_.CheckPolicySettings(policies_, &errors_);
  }

  void ApplyPolicySettings(const bool value) {
    SetPolicy(std::make_unique<base::Value>(value));
    handler_.ApplyPolicySettings(policies_, &prefs_);
  }

  SecondaryGoogleAccountSigninPolicyHandler handler_;
  PolicyMap policies_;
  PolicyErrorMap errors_;
  PrefValueMap prefs_;
};

// Test that the policy value type is a boolean.
TEST_F(SecondaryGoogleAccountSigninPolicyHandlerTest,
       CheckPolicyValueTypeIsBoolean) {
  // Should get an error for a non boolean type.
  errors_.Clear();
  EXPECT_FALSE(CheckPolicy(std::make_unique<base::ListValue>()));
  EXPECT_EQ(1U, errors_.size());
  base::string16 error =
      errors_.GetErrors(key::kSecondaryGoogleAccountSigninEnabled);
  EXPECT_EQ(base::ASCIIToUTF16("Expected boolean value."), error);

  // Should not get any error for a boolean type.
  errors_.Clear();
  EXPECT_TRUE(CheckPolicy(std::make_unique<base::Value>(true)));
  EXPECT_EQ(0U, errors_.size());
}

// Test that the handler negates the value of the policy while setting the
// preference.
TEST_F(SecondaryGoogleAccountSigninPolicyHandlerTest,
       CheckPreferenceValueIsNegated) {
  bool preference;

  ApplyPolicySettings(true /* policy value */);
  EXPECT_TRUE(
      prefs_.GetBoolean(prefs::kAccountConsistencyMirrorRequired, &preference));
  EXPECT_FALSE(preference);

  ApplyPolicySettings(false /* policy value */);
  EXPECT_TRUE(
      prefs_.GetBoolean(prefs::kAccountConsistencyMirrorRequired, &preference));
  EXPECT_TRUE(preference);
}

}  // namespace policy
