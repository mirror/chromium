// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part_chromeos.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_store_chromeos.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_store.h"
#include "components/policy/core/common/cloud/policy_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::InvokeWithoutArgs;

namespace policy {

const char kCustomDisplayDomain[] = "acme.corp";

void WaitUntilPolicyLoaded() {
  BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  DeviceCloudPolicyStoreChromeOS* store =
      connector->GetDeviceCloudPolicyManager()->device_store();
  if (!store->has_policy()) {
    MockCloudPolicyStoreObserver observer;
    base::RunLoop loop;
    store->AddObserver(&observer);
    EXPECT_CALL(observer, OnStoreLoaded(store))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(&loop, &base::RunLoop::Quit));
    loop.Run();
    store->RemoveObserver(&observer);
  }
}

class BrowserPolicyConnectorChromeOSTest : public DevicePolicyCrosBrowserTest {
};

// Sets the policy for the custom display domain and checks that it's really
// being used.
IN_PROC_BROWSER_TEST_F(BrowserPolicyConnectorChromeOSTest, DomainRename) {
  device_policy()->policy_data().set_display_domain(kCustomDisplayDomain);
  RefreshDevicePolicy();
  WaitUntilPolicyLoaded();
  BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  EXPECT_EQ(kCustomDisplayDomain, connector->GetEnterpriseDisplayDomain());
}

}  // namespace policy
