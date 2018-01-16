// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_
#define COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_

#include <memory>

#include "components/policy/policy_export.h"

namespace policy {

class PolicyErrorMap;

// Notified of policy related errors from ConfigurationPolicyPrefStore.
// Implementations will typically LOG the errors once ui::ResourceBundle has
// been loaded.
class POLICY_EXPORT ConfigurationPolicyPrefStoreErrorHandler {
 public:
  virtual void OnConfigurationPolicyErrors(
      std::unique_ptr<PolicyErrorMap> errors) = 0;

 protected:
  virtual ~ConfigurationPolicyPrefStoreErrorHandler() = default;
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_
