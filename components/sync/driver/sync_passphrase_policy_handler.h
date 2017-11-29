// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_PASSPHRASE_POLICY_HANDLER_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_PASSPHRASE_POLICY_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

class PrefValueMap;

namespace syncer {

class PolicyMap;

// ConfigurationPolicyHandler for the SyncCustomPassphraseRequired policy.
class SyncPassphrasePolicyHandler : public policy::TypeCheckingPolicyHandler {
 public:
  SyncPassphrasePolicyHandler();
  ~SyncPassphrasePolicyHandler() override;

  // ConfigurationPolicyHandler methods:
  void ApplyPolicySettings(const policy::PolicyMap& policies,
                           PrefValueMap* prefs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncPassphrasePolicyHandler);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_PASSPHRASE_POLICY_HANDLER_H_
