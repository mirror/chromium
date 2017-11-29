// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_passphrase_policy_handler.h"

#include "base/values.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/sync/base/pref_names.h"

namespace syncer {

SyncPassphrasePolicyHandler::SyncPassphrasePolicyHandler()
    : policy::TypeCheckingPolicyHandler(
          policy::key::kSyncCustomPassphraseRequired,
          base::Value::Type::BOOLEAN) {}

SyncPassphrasePolicyHandler::~SyncPassphrasePolicyHandler() {}

void SyncPassphrasePolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value = policies.GetValue(policy_name());
  bool require_custom_passphrase;
  if (value && value->GetAsBoolean(&require_custom_passphrase) &&
      require_custom_passphrase)
    prefs->SetValue(prefs::kSyncRequireCustomPassphrase,
                    value->CreateDeepCopy());
}

}  // namespace syncer
