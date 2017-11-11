// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/site_per_process_policy_handler.h"

#include "base/command_line.h"
#include "base/values.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "content/public/common/content_switches.h"

namespace policy {

SitePerProcessPolicyHandler::SitePerProcessPolicyHandler()
    : TypeCheckingPolicyHandler(key::kSitePerProcess,
                                base::Value::Type::BOOLEAN) {}

SitePerProcessPolicyHandler::~SitePerProcessPolicyHandler() {}

void SitePerProcessPolicyHandler::ApplyPolicySettings(const PolicyMap& policies,
                                                      PrefValueMap* prefs) {
  bool enable_site_per_process = false;
  const base::Value* value = policies.GetValue(policy_name());
  if (value && value->GetAsBoolean(&enable_site_per_process) &&
      enable_site_per_process) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kSitePerProcess);
  }
  DLOG(WARNING) << "policy value: " << value
                << "; enable_site_per_process: " << enable_site_per_process;
}

}  // namespace policy
