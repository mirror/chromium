// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/power/arc_power_bridge.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "components/arc/arc_bridge_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

class ArcPowerBridgeTest : public testing::Test {
 public:
  ArcPowerBridgeTest() {
    fake_power_manager_client_ = new chromeos::FakePowerManagerClient();
    if (chromeos::PowerPolicyController::IsInitialized())
      chromeos::PowerPolicyController::Shutdown();
    chromeos::PowerPolicyController::Initialize(fake_power_manager_client_);
    arc_bridge_service_ = base::MakeUnique<ArcBridgeService>();
    instance_ = base::MakeUnique<ArcPowerBridge>(nullptr /* context */,
                                                 arc_bridge_service_.get());
    chromeos::DBusThreadManager::GetSetterForTesting()->SetPowerManagerClient(
        base::WrapUnique(fake_power_manager_client_));
  }

  ~ArcPowerBridgeTest() override {
    instance_.reset();
    arc_bridge_service_.reset();
    chromeos::PowerPolicyController::Shutdown();
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  std::unique_ptr<ArcBridgeService> arc_bridge_service_;
  std::unique_ptr<ArcPowerBridge> instance_;
  chromeos::FakePowerManagerClient* fake_power_manager_client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcPowerBridgeTest);
};

TEST_F(ArcPowerBridgeTest, TestOnAcquireDisplayWakeLock) {
  mojom::DisplayWakeLockType wake_lock_type =
      mojom::DisplayWakeLockType::BRIGHT;
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  // Taking a bright wake lock should make PowerPolicyController to
  // ask for screen wake lock from powerd.
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 1);
  EXPECT_TRUE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
  // Taking a dim wake lock should make PowerPolicyController to
  // ask for dim wake lock from powerd.
  wake_lock_type = mojom::DisplayWakeLockType::DIM;
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 2);
  EXPECT_TRUE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_TRUE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
  // Release bright wake lock
  wake_lock_type = mojom::DisplayWakeLockType::BRIGHT;
  instance_->OnReleaseDisplayWakeLock(wake_lock_type);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 3);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_TRUE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
  // Release dim wake lock
  wake_lock_type = mojom::DisplayWakeLockType::DIM;
  instance_->OnReleaseDisplayWakeLock(wake_lock_type);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 4);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
}

TEST_F(ArcPowerBridgeTest, TestMultipleOnAcquireDisplayWakeLock) {
  mojom::DisplayWakeLockType wake_lock_type =
      mojom::DisplayWakeLockType::BRIGHT;
  // Taking multiple wake locks should result in policy change only once.
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 1);
  EXPECT_TRUE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
  // Releasing one wake lock after acquiring 3 should not result in the
  // request to release wake_lock owened by PowerPolicyController nor
  // policy change.
  instance_->OnReleaseDisplayWakeLock(wake_lock_type);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 1);
  EXPECT_TRUE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
  // Releasing  the rest 2  wake locks should  result in the release
  // of wake lock owned by PowerPolicyController and policy change.
  instance_->OnReleaseDisplayWakeLock(wake_lock_type);
  instance_->OnReleaseDisplayWakeLock(wake_lock_type);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 2);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());

  // Acquiring a wake lock of different type should result in a policy change.
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  instance_->OnAcquireDisplayWakeLock(wake_lock_type);
  instance_->OnAcquireDisplayWakeLock(mojom::DisplayWakeLockType::DIM);
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 4);
  EXPECT_TRUE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_TRUE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
}

TEST_F(ArcPowerBridgeTest, TestOnAcquirePartialWakeLock) {
  // Taking a partial wake lock should make PowerPolicyController to
  // ask for system wake lock from powerd.
  instance_->OnAcquirePartialWakeLock();
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 1);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_TRUE(fake_power_manager_client_->policy().system_wake_lock());
  // Release partial wake lock
  instance_->OnReleasePartialWakeLock();
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 2);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
}

TEST_F(ArcPowerBridgeTest, TestMultipleOnAcquirePartialWakeLock) {
  // Taking multiple wake locks should result in policy change only once.
  instance_->OnAcquirePartialWakeLock();
  instance_->OnAcquirePartialWakeLock();
  instance_->OnAcquirePartialWakeLock();
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 1);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_TRUE(fake_power_manager_client_->policy().system_wake_lock());
  // Releasing one wake lock after acquiring 3 should not result in the
  // request to release wake_lock owened by PowerPolicyController nor
  // policy change.
  instance_->OnReleasePartialWakeLock();
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 1);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_TRUE(fake_power_manager_client_->policy().system_wake_lock());
  // Releasing  the rest 2  wake locks should  result in the release
  // of wake lock owned by PowerPolicyController and policy change.
  instance_->OnReleasePartialWakeLock();
  instance_->OnReleasePartialWakeLock();
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 2);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
}

TEST_F(ArcPowerBridgeTest, TestReleaseAllWakeLocks) {
  // Taking multiple wake locks should result in policy change only once.
  instance_->OnAcquirePartialWakeLock();
  instance_->OnAcquirePartialWakeLock();
  instance_->OnAcquirePartialWakeLock();
  instance_->OnAcquireDisplayWakeLock(mojom::DisplayWakeLockType::DIM);
  instance_->OnAcquireDisplayWakeLock(mojom::DisplayWakeLockType::BRIGHT);
  instance_->OnInstanceClosed();
  EXPECT_EQ(fake_power_manager_client_->num_set_policy_calls(), 6);
  EXPECT_FALSE(fake_power_manager_client_->policy().screen_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().dim_wake_lock());
  EXPECT_FALSE(fake_power_manager_client_->policy().system_wake_lock());
}

}  // namespace arc
