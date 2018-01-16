// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_TEST_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_
#define COMPONENTS_POLICY_CORE_BROWSER_TEST_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_

#include "base/macros.h"
#include "components/policy/core/browser/configuration_policy_pref_store_error_handler.h"

namespace policy {

class TestConfigurationPolicyPrefStoreErrorHandler
    : public ConfigurationPolicyPrefStoreErrorHandler {
 public:
  TestConfigurationPolicyPrefStoreErrorHandler();
  ~TestConfigurationPolicyPrefStoreErrorHandler() override;

  // ConfigurationPolicyPrefStoreErrorHandler override:
  void OnConfigurationPolicyErrors(
      std::unique_ptr<PolicyErrorMap> errors) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestConfigurationPolicyPrefStoreErrorHandler);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_TEST_CONFIGURATION_POLICY_PREF_STORE_ERROR_HANDLER_H_
