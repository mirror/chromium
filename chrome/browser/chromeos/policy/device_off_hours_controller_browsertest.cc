// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

#include <utility>

#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"

namespace em = enterprise_management;

namespace {
static std::vector<std::string> kDefaultStringVector;
static std::vector<em::DeviceOffHoursIntervalProto> kDefaultIntervalVector;
}  // namespace

namespace policy {

class DeviceOffHoursPolicyControllerTest
    : public policy::DevicePolicyCrosBrowserTest {
 public:
  DeviceOffHoursPolicyControllerTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    InstallOwnerKey();
    MarkAsEnterpriseOwned();
    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
  }

 protected:
  void SetPolicy(
      const std::vector<em::DeviceOffHoursIntervalProto>& intervals,
      const std::string& timezone = "GMT",
      const std::vector<std::string>& ignored_policy = kDefaultStringVector) {
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    auto* off_hours = proto.mutable_device_off_hours();
    for (auto interval : intervals) {
      auto* cur = off_hours->add_interval();
      *cur = interval;
    }
    off_hours->set_timezone(timezone);
    for (auto p : ignored_policy) {
      off_hours->add_policy(p);
    }
    RefreshPolicyAndWaitUntilDeviceSettingsUpdated();
  }

  void UnsetPolicy() {
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    proto.clear_device_off_hours();
    RefreshPolicyAndWaitUntilDeviceSettingsUpdated();
  }

  void RefreshPolicyAndWaitUntilDeviceSettingsUpdated() {
    base::RunLoop run_loop;
    std::unique_ptr<chromeos::CrosSettings::ObserverSubscription> observer =
        chromeos::CrosSettings::Get()->AddSettingsObserver(
            chromeos::kDeviceOffHours, run_loop.QuitClosure());
    RefreshDevicePolicy();
    run_loop.Run();
  }
};

IN_PROC_BROWSER_TEST_F(DeviceOffHoursPolicyControllerTest,
                       SetOffHoursTimezone) {
  // TODO(yakovleva) more timezones
  std::string tz = "ROC";
  SetPolicy(kDefaultIntervalVector, tz);
  em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
  EXPECT_TRUE(proto.has_device_off_hours());
  EXPECT_TRUE(proto.device_off_hours().has_timezone());
  EXPECT_EQ(tz, proto.device_off_hours().timezone());
}
}  // namespace policy