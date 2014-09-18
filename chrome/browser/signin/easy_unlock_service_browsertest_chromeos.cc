// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/ui/user_adding_screen.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/easy_unlock_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/user_manager/user_manager.h"
#include "content/public/common/content_switches.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "extensions/browser/extension_system.h"
#include "policy/policy_constants.h"
#include "testing/gmock/include/gmock/gmock.h"

using chromeos::ProfileHelper;
using chromeos::LoginManagerTest;
using chromeos::StartupUtils;
using chromeos::UserAddingScreen;
using user_manager::UserManager;
using device::MockBluetoothAdapter;
using testing::_;
using testing::Return;

namespace {

const char kTestUser1[] = "primary.user@example.com";
const char kTestUser2[] = "secondary.user@example.com";

#if defined(GOOGLE_CHROME_BUILD)
bool HasEasyUnlockAppForProfile(Profile* profile) {
  extensions::ExtensionSystem* extension_system =
      extensions::ExtensionSystem::Get(profile);
  ExtensionService* extension_service = extension_system->extension_service();
  return !!extension_service->GetExtensionById(
      extension_misc::kEasyUnlockAppId, false);
}
#endif

void SetUpBluetoothMock(
    scoped_refptr<testing::NiceMock<MockBluetoothAdapter> > mock_adapter,
    bool is_present) {
  device::BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);

  EXPECT_CALL(*mock_adapter, IsPresent())
      .WillRepeatedly(testing::Return(is_present));

  // These functions are called from ash system tray. They are speculations of
  // why flaky gmock errors are seen on bots.
  EXPECT_CALL(*mock_adapter, IsPowered())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*mock_adapter, GetDevices()).WillRepeatedly(
      testing::Return(device::BluetoothAdapter::ConstDeviceList()));
}

}  // namespace

class EasyUnlockServiceTest : public InProcessBrowserTest {
 public:
  EasyUnlockServiceTest() : is_bluetooth_adapter_present_(true) {}
  virtual ~EasyUnlockServiceTest() {}

  void SetEasyUnlockAllowedPolicy(bool allowed) {
    policy::PolicyMap policy;
    policy.Set(policy::key::kEasyUnlockAllowed,
               policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER,
               new base::FundamentalValue(allowed),
               NULL);
    provider_.UpdateChromePolicy(policy);
    base::RunLoop().RunUntilIdle();
  }

#if defined(GOOGLE_CHROME_BUILD)
  bool HasEasyUnlockApp() const {
    return HasEasyUnlockAppForProfile(profile());
  }
#endif

  // InProcessBrowserTest:
  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    mock_adapter_ = new testing::NiceMock<MockBluetoothAdapter>();
    SetUpBluetoothMock(mock_adapter_, is_bluetooth_adapter_present_);
  }

  Profile* profile() const { return browser()->profile(); }

  EasyUnlockService* service() const {
    return EasyUnlockService::Get(profile());
  }

  void set_is_bluetooth_adapter_present(bool is_present) {
    is_bluetooth_adapter_present_ = is_present;
  }

 private:
  policy::MockConfigurationPolicyProvider provider_;
  scoped_refptr<testing::NiceMock<MockBluetoothAdapter> > mock_adapter_;
  bool is_bluetooth_adapter_present_;

  DISALLOW_COPY_AND_ASSIGN(EasyUnlockServiceTest);
};

// Tests that EasyUnlock is on by default.
IN_PROC_BROWSER_TEST_F(EasyUnlockServiceTest, DefaultOn) {
  EXPECT_TRUE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_TRUE(HasEasyUnlockApp());
#endif
}

class EasyUnlockServiceNoBluetoothTest : public EasyUnlockServiceTest {
 public:
  EasyUnlockServiceNoBluetoothTest() {}
  virtual ~EasyUnlockServiceNoBluetoothTest() {}

  // InProcessBrowserTest:
  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    set_is_bluetooth_adapter_present(false);
    EasyUnlockServiceTest::SetUpInProcessBrowserTestFixture();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EasyUnlockServiceNoBluetoothTest);
};

IN_PROC_BROWSER_TEST_F(EasyUnlockServiceNoBluetoothTest, NoService) {
  EXPECT_FALSE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_FALSE(HasEasyUnlockApp());
#endif
}

class EasyUnlockServiceFinchEnabledTest : public EasyUnlockServiceTest {
 public:
  EasyUnlockServiceFinchEnabledTest() {}
  virtual ~EasyUnlockServiceFinchEnabledTest() {}

  // InProcessBrowserTest:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    command_line->AppendSwitchASCII(switches::kForceFieldTrials,
                                    "EasyUnlock/Enable/");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EasyUnlockServiceFinchEnabledTest);
};

// Tests that policy can override finch to turn easy unlock off.
IN_PROC_BROWSER_TEST_F(EasyUnlockServiceFinchEnabledTest, PolicyOveride) {
  EXPECT_TRUE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_TRUE(HasEasyUnlockApp());
#endif

  // Overridden by policy.
  SetEasyUnlockAllowedPolicy(false);
  EXPECT_FALSE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_FALSE(HasEasyUnlockApp());
#endif

  SetEasyUnlockAllowedPolicy(true);
  EXPECT_TRUE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_TRUE(HasEasyUnlockApp());
#endif
}

class EasyUnlockServiceFinchDisabledTest : public EasyUnlockServiceTest {
 public:
  EasyUnlockServiceFinchDisabledTest() {}
  virtual ~EasyUnlockServiceFinchDisabledTest() {}

  // InProcessBrowserTest:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    command_line->AppendSwitchASCII(switches::kForceFieldTrials,
                                    "EasyUnlock/Disable/");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EasyUnlockServiceFinchDisabledTest);
};

// Tests that easy unlock is off when finch is disabled and policy overrides
// finch.
IN_PROC_BROWSER_TEST_F(EasyUnlockServiceFinchDisabledTest, PolicyOverride) {
  // Finch is disabled.
  EXPECT_FALSE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_FALSE(HasEasyUnlockApp());
#endif

  // Policy overrides finch and turns on Easy unlock.
  SetEasyUnlockAllowedPolicy(true);
  EXPECT_TRUE(service()->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_TRUE(HasEasyUnlockApp());
#endif
}

class EasyUnlockServiceMultiProfileTest : public LoginManagerTest {
 public:
  EasyUnlockServiceMultiProfileTest() : LoginManagerTest(false) {}
  virtual ~EasyUnlockServiceMultiProfileTest() {}

  // InProcessBrowserTest:
  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    LoginManagerTest::SetUpInProcessBrowserTestFixture();

    mock_adapter_ = new testing::NiceMock<MockBluetoothAdapter>();
    SetUpBluetoothMock(mock_adapter_, true);
  }

 private:
  scoped_refptr<testing::NiceMock<MockBluetoothAdapter> > mock_adapter_;
  DISALLOW_COPY_AND_ASSIGN(EasyUnlockServiceMultiProfileTest);
};

IN_PROC_BROWSER_TEST_F(EasyUnlockServiceMultiProfileTest,
                       PRE_DisallowedOnSecondaryProfile) {
  RegisterUser(kTestUser1);
  RegisterUser(kTestUser2);
  StartupUtils::MarkOobeCompleted();
}

IN_PROC_BROWSER_TEST_F(EasyUnlockServiceMultiProfileTest,
                       DisallowedOnSecondaryProfile) {
  LoginUser(kTestUser1);
  chromeos::UserAddingScreen::Get()->Start();
  base::RunLoop().RunUntilIdle();
  AddUser(kTestUser2);
  const user_manager::User* primary_user =
      user_manager::UserManager::Get()->FindUser(kTestUser1);
  const user_manager::User* secondary_user =
      user_manager::UserManager::Get()->FindUser(kTestUser2);

  Profile* primary_profile = ProfileHelper::Get()->GetProfileByUserIdHash(
      primary_user->username_hash());
  Profile* secondary_profile = ProfileHelper::Get()->GetProfileByUserIdHash(
      secondary_user->username_hash());

  EXPECT_TRUE(EasyUnlockService::Get(primary_profile)->IsAllowed());
  EXPECT_FALSE(EasyUnlockService::Get(secondary_profile)->IsAllowed());
#if defined(GOOGLE_CHROME_BUILD)
  EXPECT_TRUE(HasEasyUnlockAppForProfile(primary_profile));
  EXPECT_FALSE(HasEasyUnlockAppForProfile(secondary_profile));
#endif
}
