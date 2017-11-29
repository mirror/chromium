// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_passphrase_policy_handler.h"

#include <memory>

#include "base/values.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/sync/base/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

// Test cases for the Sync policy setting.
class SyncPassphrasePolicyHandlerTest : public testing::Test {};

TEST_F(SyncPassphrasePolicyHandlerTest, Default) {
  policy::PolicyMap policy;
  SyncPassphrasePolicyHandler handler;
  PrefValueMap prefs;
  handler.ApplyPolicySettings(policy, &prefs);
  EXPECT_FALSE(prefs.GetValue(prefs::kSyncRequireCustomPassphrase, nullptr));
}

TEST_F(SyncPassphrasePolicyHandlerTest, Enabled) {
  policy::PolicyMap policy;
  policy.Set(policy::key::kSyncCustomPassphraseRequired,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(false),
             nullptr);
  SyncPassphrasePolicyHandler handler;
  PrefValueMap prefs;
  handler.ApplyPolicySettings(policy, &prefs);

  // Enabling Sync should not set the pref.
  EXPECT_FALSE(prefs.GetValue(prefs::kSyncRequireCustomPassphrase, nullptr));
}

TEST_F(SyncPassphrasePolicyHandlerTest, Disabled) {
  policy::PolicyMap policy;
  policy.Set(policy::key::kSyncCustomPassphraseRequired,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(true),
             nullptr);
  SyncPassphrasePolicyHandler handler;
  PrefValueMap prefs;
  handler.ApplyPolicySettings(policy, &prefs);

  // Sync should be flagged as managed.
  const base::Value* value = nullptr;
  EXPECT_TRUE(prefs.GetValue(prefs::kSyncRequireCustomPassphrase, &value));
  ASSERT_TRUE(value);
  bool sync_custom_passphrase_required = false;
  bool result = value->GetAsBoolean(&sync_custom_passphrase_required);
  ASSERT_TRUE(result);
  EXPECT_TRUE(sync_custom_passphrase_required);
}

}  // namespace syncer
