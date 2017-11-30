// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/display_forced_off_setter.h"

#include <memory>
#include <utility>
#include <vector>

#include "ash/system/power/scoped_display_forced_off.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/test/test_message_loop.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

namespace {

class TestObserver : public DisplayForcedOffSetter::Observer {
 public:
  explicit TestObserver(DisplayForcedOffSetter* display_forced_off_setter)
      : display_forced_off_setter_(display_forced_off_setter),
        scoped_observer_(this) {
    scoped_observer_.Add(display_forced_off_setter);
  }

  ~TestObserver() override = default;

  // DisplayForcedOffSetter::Observer:
  void OnBacklightsForcedOffChanged(bool backlights_forced_off) override {
    ASSERT_EQ(backlights_forced_off,
              display_forced_off_setter_->backlights_forced_off());
    forced_off_states_.push_back(backlights_forced_off);
  }

  const std::vector<bool>& forced_off_states() const {
    return forced_off_states_;
  }

  void ClearForcedOffStates() { forced_off_states_.clear(); }

  void StopObserving() { scoped_observer_.RemoveAll(); }

 private:
  DisplayForcedOffSetter* const display_forced_off_setter_;

  std::vector<bool> forced_off_states_;

  ScopedObserver<DisplayForcedOffSetter, DisplayForcedOffSetter::Observer>
      scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace

class DisplayForcedOffSetterTest : public testing::Test {
 public:
  DisplayForcedOffSetterTest() = default;
  ~DisplayForcedOffSetterTest() override = default;

  void SetUp() override {
    auto power_manager_client =
        std::make_unique<chromeos::FakePowerManagerClient>();
    power_manager_client_ = power_manager_client.get();
    chromeos::DBusThreadManager::GetSetterForTesting()->SetPowerManagerClient(
        std::move(power_manager_client));

    display_forced_off_setter_ = std::make_unique<DisplayForcedOffSetter>();
    display_forced_off_observer_ =
        std::make_unique<TestObserver>(display_forced_off_setter_.get());
  }

  void TearDown() override {
    display_forced_off_observer_.reset();
    display_forced_off_setter_.reset();
    chromeos::DBusThreadManager::Shutdown();
  }

  chromeos::FakePowerManagerClient* power_manager_client() {
    return power_manager_client_;
  }

  DisplayForcedOffSetter* display_forced_off_setter() {
    return display_forced_off_setter_.get();
  }

  TestObserver* display_forced_off_observer() {
    return display_forced_off_observer_.get();
  }

  void ResetDisplayForcedOffSetter() {
    display_forced_off_observer_->StopObserving();
    display_forced_off_setter_.reset();
  }

 private:
  base::TestMessageLoop message_loop_;

  chromeos::FakePowerManagerClient* power_manager_client_ = nullptr;

  std::unique_ptr<DisplayForcedOffSetter> display_forced_off_setter_;
  std::unique_ptr<TestObserver> display_forced_off_observer_;

  DISALLOW_COPY_AND_ASSIGN(DisplayForcedOffSetterTest);
};

TEST_F(DisplayForcedOffSetterTest, SingleForcedOffRequest) {
  ASSERT_FALSE(power_manager_client()->backlights_forced_off());

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off =
      display_forced_off_setter()->ForceDisplayOff();

  EXPECT_TRUE(scoped_forced_off->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({true}),
            display_forced_off_observer()->forced_off_states());
  display_forced_off_observer()->ClearForcedOffStates();

  scoped_forced_off->Reset();

  EXPECT_FALSE(scoped_forced_off->IsActive());

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({false}),
            display_forced_off_observer()->forced_off_states());
}

TEST_F(DisplayForcedOffSetterTest,
       SingleForcedOffRequest_RequestGoesOutOfScope) {
  ASSERT_FALSE(power_manager_client()->backlights_forced_off());

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off =
      display_forced_off_setter()->ForceDisplayOff();

  EXPECT_TRUE(scoped_forced_off->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({true}),
            display_forced_off_observer()->forced_off_states());
  display_forced_off_observer()->ClearForcedOffStates();

  scoped_forced_off.reset();

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({false}),
            display_forced_off_observer()->forced_off_states());
}

TEST_F(DisplayForcedOffSetterTest, DisplayForcedOffSetterDeleted) {
  ASSERT_FALSE(power_manager_client()->backlights_forced_off());

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off =
      display_forced_off_setter()->ForceDisplayOff();

  EXPECT_TRUE(scoped_forced_off->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({true}),
            display_forced_off_observer()->forced_off_states());
  display_forced_off_observer()->ClearForcedOffStates();

  ResetDisplayForcedOffSetter();

  EXPECT_FALSE(scoped_forced_off->IsActive());
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());

  // Verify that deleting scoped forced off request does not affect
  // power manager state (nor cause a crash).
  scoped_forced_off.reset();
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
}

TEST_F(DisplayForcedOffSetterTest,
       OverlappingRequests_SecondRequestResetFirst) {
  ASSERT_FALSE(power_manager_client()->backlights_forced_off());

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off_1 =
      display_forced_off_setter()->ForceDisplayOff();
  EXPECT_TRUE(scoped_forced_off_1->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({true}),
            display_forced_off_observer()->forced_off_states());
  display_forced_off_observer()->ClearForcedOffStates();

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off_2 =
      display_forced_off_setter()->ForceDisplayOff();
  EXPECT_TRUE(scoped_forced_off_2->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_TRUE(display_forced_off_observer()->forced_off_states().empty());

  scoped_forced_off_2.reset();

  EXPECT_TRUE(scoped_forced_off_1->IsActive());
  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_TRUE(display_forced_off_observer()->forced_off_states().empty());

  scoped_forced_off_1.reset();

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({false}),
            display_forced_off_observer()->forced_off_states());
}

TEST_F(DisplayForcedOffSetterTest, OverlappingRequests_FirstRequestResetFirst) {
  ASSERT_FALSE(power_manager_client()->backlights_forced_off());

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off_1 =
      display_forced_off_setter()->ForceDisplayOff();
  EXPECT_TRUE(scoped_forced_off_1->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({true}),
            display_forced_off_observer()->forced_off_states());
  display_forced_off_observer()->ClearForcedOffStates();

  std::unique_ptr<ScopedDisplayForcedOff> scoped_forced_off_2 =
      display_forced_off_setter()->ForceDisplayOff();
  EXPECT_TRUE(scoped_forced_off_2->IsActive());

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_TRUE(display_forced_off_observer()->forced_off_states().empty());

  scoped_forced_off_1.reset();

  EXPECT_TRUE(scoped_forced_off_2->IsActive());
  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_TRUE(display_forced_off_observer()->forced_off_states().empty());

  scoped_forced_off_2.reset();

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(std::vector<bool>({false}),
            display_forced_off_observer()->forced_off_states());
}

}  // namespace ash
