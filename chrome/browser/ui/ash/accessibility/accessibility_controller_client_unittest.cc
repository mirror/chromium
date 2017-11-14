// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/accessibility/accessibility_controller_client.h"

#include "ash/public/interfaces/accessibility_controller.mojom.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/signin/core/account_id/account_id.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTestUser[] = "test@test.com";

class TestAccessibilityController : ash::mojom::AccessibilityController {
 public:
  TestAccessibilityController() : binding_(this) {}
  ~TestAccessibilityController() override = default;

  ash::mojom::AccessibilityControllerPtr CreateInterfacePtr() {
    ash::mojom::AccessibilityControllerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // ash::mojom::AccessibilityController:
  void SetClient(ash::mojom::AccessibilityControllerClientPtr client) override {
    was_client_set_ = true;
  }

  bool was_client_set() const { return was_client_set_; }

 private:
  mojo::Binding<ash::mojom::AccessibilityController> binding_;
  bool was_client_set_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestAccessibilityController);
};

}  // namespace

class AccessibilityControllerClientTest : public testing::Test {
 protected:
  AccessibilityControllerClientTest() = default;
  ~AccessibilityControllerClientTest() override = default;

  void SetUp() override {
    testing::Test::SetUp();

    user_manager_ = new chromeos::FakeChromeUserManager;
    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        base::WrapUnique(user_manager_));

    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
  }

  void TearDown() override {
    user_manager_enabler_.reset();
    user_manager_ = nullptr;
    profile_manager_.reset();
    testing::Test::TearDown();
  }

  // Owned by |user_manager_enabler_|.
  chromeos::FakeChromeUserManager* user_manager_ = nullptr;

  std::unique_ptr<TestingProfileManager> profile_manager_;

 private:
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerClientTest);
};

TEST_F(AccessibilityControllerClientTest, TriggerAccessibilityAlert) {
  AccessibilityControllerClient client;
  TestAccessibilityController controller;
  client.InitForTesting(controller.CreateInterfacePtr());
  client.FlushForTesting();

  // Tests that singleton was initialized and client is set.
  EXPECT_EQ(&client, AccessibilityControllerClient::Get());
  EXPECT_TRUE(controller.was_client_set());

  const AccountId account_id(AccountId::FromUserEmail(kTestUser));
  user_manager_->AddUser(account_id);
  user_manager_->LoginUser(account_id);
  profile_manager_->CreateTestingProfile(kTestUser);

  // Tests TriggerAccessibilityAlert method call.
  const ash::mojom::AccessibilityAlert alert =
      ash::mojom::AccessibilityAlert::SCREEN_ON;
  client.TriggerAccessibilityAlert(alert);
  EXPECT_EQ(alert, client.last_a11y_alert_for_test());
}
