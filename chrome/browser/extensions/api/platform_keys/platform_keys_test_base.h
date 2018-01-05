// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PLATFORM_KEYS_PLATFORM_KEYS_TEST_BASE_H_
#define CHROME_BROWSER_EXTENSIONS_API_PLATFORM_KEYS_PLATFORM_KEYS_TEST_BASE_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/test/https_forwarder.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/signin/core/account_id/account_id.h"
#include "google_apis/gaia/fake_gaia.h"

namespace crypto {
class ScopedTestSystemNSSKeySlot;
}

// An ExtensionApiTest which provides additional setup for system token
// availability, device enrollment status, user affiliation and user policy.
class PlatformKeysTestBase : public ExtensionApiTest {
 public:
  enum class SystemToken { EXISTS, DOES_NOT_EXIST };

  enum class EnrollmentStatus { ENROLLED, NOT_ENROLLED };

  enum class UserStatus {
    UNMANAGED,
    MANAGED_AFFILIATED_DOMAIN,
    MANAGED_OTHER_DOMAIN
  };

  PlatformKeysTestBase();
  ~PlatformKeysTestBase() override;

 protected:
  void SetUp() override;
  void SetUpCommandLine(base::CommandLine* command_line) override;
  // InProcessBrowserTest:
  void SetUpInProcessBrowserTestFixture() override;
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

  const AccountId account_id_;

  virtual SystemToken GetSystemToken() = 0;
  virtual EnrollmentStatus GetEnrollmentStatus() = 0;
  virtual UserStatus GetUserStatus() = 0;

  virtual void PrepareTestSystemSlotOnIO(
      crypto::ScopedTestSystemNSSKeySlot* slot);

  void RunPreTest();
  bool TestExtension(Browser* browser, const std::string& page_url);

  bool IsPreTest();

  void SetUpTestSystemSlotOnIO(base::OnceClosure done_callback);
  void TearDownTestSystemSlotOnIO(base::OnceClosure done_callback);

  policy::DevicePolicyCrosTestHelper device_policy_test_helper_;
  std::unique_ptr<crypto::ScopedTestSystemNSSKeySlot> test_system_slot_;
  policy::MockConfigurationPolicyProvider policy_provider_;
  FakeGaia fake_gaia_;
  chromeos::HTTPSForwarder gaia_https_forwarder_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformKeysTestBase);
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_PLATFORM_KEYS_PLATFORM_KEYS_TEST_BASE_H_
