// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_
#define COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_

#include <memory>

namespace policy {

class PolicyErrorMap;

class ConfigurationPolicyPrefStoreErrorHandler {
 public:
  virtual void OnConfigurationPolicyErrors(
      std::unique_ptr<PolicyErrorMap> errors) = 0;

 protected:
  virtual ~ConfigurationPolicyPrefStoreErrorHandler() = default;
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_
