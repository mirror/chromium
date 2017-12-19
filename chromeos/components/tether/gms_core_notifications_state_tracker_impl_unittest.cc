// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/gms_core_notifications_state_tracker_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

class TestObserver final : public GmsCoreNotificationsStateTracker::Observer {
 public:
  explicit TestObserver(GmsCoreNotificationsStateTrackerImpl* tracker)
      : tracker_(tracker) {}

  ~TestObserver() = default;

  uint32_t change_count() const { return change_count_; }

  const std::unordered_set<std::string>& names_from_last_update() {
    return names_from_last_update_;
  }

  // GmsCoreNotificationsStateTracker::Observer:
  void OnGmsCoreNotificationStateChanged() override {
    names_from_last_update_ =
        tracker_->GetGmsCoreNotificationsDisabledDeviceNames();
    ++change_count_;
  }

 private:
  GmsCoreNotificationsStateTrackerImpl* tracker_;

  uint32_t change_count_ = 0;
  std::unordered_set<std::string> names_from_last_update_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace

class GmsCoreNotificationsStateTrackerImplTest : public testing::Test {
 protected:
  GmsCoreNotificationsStateTrackerImplTest() {}

  void SetUp() override {
    device_names_to_send_.clear();
    tracker_ = std::make_unique<GmsCoreNotificationsStateTrackerImpl>();

    observer_ = std::make_unique<TestObserver>(tracker_.get());
    tracker_->AddObserver(observer_.get());
  }

  void VerifyExpectedNames(const std::unordered_set<std::string> expected_names,
                           size_t expected_change_count) {
    EXPECT_EQ(expected_names, observer_->names_from_last_update());
    EXPECT_EQ(expected_change_count, observer_->change_count());
  }

  void ReceiveTetherAvailabilityResponse(bool is_final_scan_result) {
    tracker_->OnTetherAvailabilityResponse(
        std::vector<HostScannerOperation::ScannedDeviceInfo>(),
        device_names_to_send_, is_final_scan_result);
  }

  std::vector<std::string> device_names_to_send_;

  std::unique_ptr<GmsCoreNotificationsStateTrackerImpl> tracker_;
  std::unique_ptr<TestObserver> observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(GmsCoreNotificationsStateTrackerImplTest);
};

TEST_F(GmsCoreNotificationsStateTrackerImplTest, TestTracking) {
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames(std::unordered_set<std::string>(),
                      0 /* expected_change_count */);

  // Add two devices and verify that they are now tracked.
  device_names_to_send_.push_back("device1");
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames({"device1"} /* expected_names */,
                      1 /* expected_change_count */);
  device_names_to_send_.push_back("device2");
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device2"} /* expected_names */,
                      2 /* expected_change_count */);

  // Receive another response with the same list; this should not result in an
  // additional "state changed" event.
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device2"} /* expected_names */,
                      2 /* expected_change_count */);

  // End the scan session.
  ReceiveTetherAvailabilityResponse(true /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device2"} /* expected_names */,
                      2 /* expected_change_count */);

  // Start a new session; since the previous session contains "device1" and
  // "device2", these names should remain at least until the scan session ends.
  device_names_to_send_.clear();
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device2"} /* expected_names */,
                      2 /* expected_change_count */);

  // Add two devices (one new and one from a previous session).
  device_names_to_send_.push_back("device1");
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device2"} /* expected_names */,
                      2 /* expected_change_count */);
  device_names_to_send_.push_back("device3");
  ReceiveTetherAvailabilityResponse(false /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device2", "device3"} /* expected_names */,
                      3 /* expected_change_count */);

  // End the scan session; since "session2" was not present in this session, it
  // should be removed.
  ReceiveTetherAvailabilityResponse(true /* is_final_scan_result */);
  VerifyExpectedNames({"device1", "device3"} /* expected_names */,
                      4 /* expected_change_count */);

  // Now, destroy |tracker_|; this should result in one more change event.
  tracker_.reset();
  VerifyExpectedNames({} /* expected_names */, 5 /* expected_change_count */);
}

}  // namespace tether

}  // namespace chromeos
