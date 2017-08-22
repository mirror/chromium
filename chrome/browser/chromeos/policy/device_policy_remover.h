// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_POLICY_REMOVER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_POLICY_REMOVER_H_

#include <memory>
#include <set>

#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"

namespace policy {

// Return a proto with cleared |policies_to_remove| fields.
// Set |policies_to_remove| contains device policy names from
// ChromeDeviceSettingsProto Implementation of this method is generated
// automatically from generate_device_policy_remover.py
enterprise_management::ChromeDeviceSettingsProto RemovePolicies(
    const enterprise_management::ChromeDeviceSettingsProto& input_policies,
    const std::set<std::string>& policies_to_remove);

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_POLICY_REMOVER_H_
