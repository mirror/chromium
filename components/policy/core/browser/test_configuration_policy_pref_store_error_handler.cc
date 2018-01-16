// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/browser/test_configuration_policy_pref_store_error_handler.h"

#include "components/policy/core/browser/policy_error_map.h"

namespace policy {

TestConfigurationPolicyPrefStoreErrorHandler::
    TestConfigurationPolicyPrefStoreErrorHandler() = default;

TestConfigurationPolicyPrefStoreErrorHandler::
    ~TestConfigurationPolicyPrefStoreErrorHandler() = default;

void TestConfigurationPolicyPrefStoreErrorHandler::OnConfigurationPolicyErrors(
    std::unique_ptr<PolicyErrorMap> errors) {}

}  // namespace policy
