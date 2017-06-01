// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/scheduler/scheduler_impl.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "components/download/internal/entry.h"
#include "components/download/internal/scheduler/device_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::InSequence;

namespace download {
namespace {

class MockPlatformTaskScheduler : public PlatformTaskScheduler {
 public:
  MOCK_METHOD2(ScheduleDownloadTask, void(bool, bool));
};

class DownloadSchedulerImplTest : public testing::Test {
 public:
  DownloadSchedulerImplTest() {}
  ~DownloadSchedulerImplTest() override = default;

  void TearDown() override { DestroyScheduler(); }

  void BuildScheduler(const std::vector<int> clients) {
    scheduler_ =
        base::MakeUnique<SchedulerImpl>(&platform_task_scheduler_, clients);
  }
  void DestroyScheduler() { scheduler_.reset(); }

  // Helper function to create a list of entries for the scheduler to query the
  // next entry.
  void BuildDataEntries(size_t size) {
    entries_ = std::vector<Entry>(size, Entry());
    for (size_t i = 0; i < size; ++i) {
      entries_[i].guid = base::IntToString(i);
      entries_[i].scheduling_params.battery_requirements =
          SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
      entries_[i].scheduling_params.network_requirements =
          SchedulingParams::NetworkRequirements::UNMETERED;
      entries_[i].state = Entry::State::AVAILABLE;
    }
  }

  // Returns list of entry pointers to feed to the scheduler.
  Model::EntryList entries() {
    Model::EntryList entry_list;
    for (auto& entry : entries_) {
      entry_list.emplace_back(&entry);
    }
    return entry_list;
  }

  // Simulates the entry has been processed by the download service and the
  // state has changed.
  void MakeEntryActive(Entry* entry) { entry->state = Entry::State::ACTIVE; }

  // Reverts the states of entry so that the scheduler can poll it again.
  void MakeEntryAvailable(Entry* entry) {
    entry->state = Entry::State::AVAILABLE;
  }

  // Helper function to build a device status.
  DeviceStatus BuildDeviceStatus(BatteryStatus battery, NetworkStatus network) {
    DeviceStatus device_status;
    device_status.battery_status = battery;
    device_status.network_status = network;
    return device_status;
  }

  std::unique_ptr<SchedulerImpl> scheduler_;
  MockPlatformTaskScheduler platform_task_scheduler_;

  // Entries owned by the test fixture.
  std::vector<Entry> entries_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadSchedulerImplTest);
};

// Ensures normal polling logic is correct.
TEST_F(DownloadSchedulerImplTest, BasicPolling) {
  BuildScheduler(std::vector<int>{1, 0});

  // Client 0: entry 0.
  // Client 1: entry 1.
  // Poll sequence: 1 -> 0.
  BuildDataEntries(2);
  entries_[0].client = static_cast<DownloadClient>(0);
  entries_[1].client = static_cast<DownloadClient>(1);

  // First download belongs to first client.
  Entry* next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(next, &entries_[1]);

  // If the first one is processed, the next should be the other entry.
  MakeEntryActive(next);
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(next, &entries_[0]);
  MakeEntryActive(next);
}

// Tests the load balancing and polling downloads based on create time.
TEST_F(DownloadSchedulerImplTest, BasicLoadBalancing) {
  BuildScheduler(std::vector<int>{0, 1, 2});

  // Client 0: entry 0, entry 1 (earlier create time).
  // Client 1: entry 2.
  // Client 2: No entries.
  // Poll sequence: 1 -> 2 -> 0.
  BuildDataEntries(3);
  entries_[0].client = static_cast<DownloadClient>(0);
  entries_[0].create_time += base::TimeDelta::FromMilliseconds(20);
  entries_[1].client = static_cast<DownloadClient>(0);
  entries_[1].create_time += base::TimeDelta::FromMilliseconds(10);
  entries_[2].client = static_cast<DownloadClient>(1);
  entries_[2].create_time += base::TimeDelta::FromMilliseconds(30);

  // There are 2 downloads for client 0, the one with earlier create time will
  // be the next download.
  Entry* next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[1], next);
  MakeEntryActive(next);

  // The second download should belongs to client 1, because of the round robin
  // load balancing.
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[2], next);
  MakeEntryActive(next);

  // Only one entry left, which will be the next.
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[0], next);
  MakeEntryActive(next);

  // Keep polling twice, since no available downloads, both will return nullptr.
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(nullptr, next);
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(nullptr, next);
}

// Ensures downloads are polled based on scheduling parameters and device
// status.
TEST_F(DownloadSchedulerImplTest, SchedulingParams) {
  BuildScheduler(std::vector<int>{0});
  BuildDataEntries(1);
  entries_[0].client = static_cast<DownloadClient>(0);
  entries_[0].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
  entries_[0].scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::UNMETERED;

  Entry* next = nullptr;

  // Tests network scheduling parameter.
  // No downloads can be polled when network disconnected.
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::DISCONNECTED));
  EXPECT_EQ(nullptr, next);

  // If the network is metered, and scheduling parameter requires unmetered
  // network, the download should not be polled.
  next = scheduler_->Next(entries(), BuildDeviceStatus(BatteryStatus::CHARGING,
                                                       NetworkStatus::METERED));
  EXPECT_EQ(nullptr, next);

  // If the network requirement is none, the download can happen under metered
  // network. However, download won't happen when network is disconnected.
  entries_[0].scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::NONE;
  next = scheduler_->Next(entries(), BuildDeviceStatus(BatteryStatus::CHARGING,
                                                       NetworkStatus::METERED));
  EXPECT_EQ(&entries_[0], next);
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::DISCONNECTED));
  EXPECT_EQ(nullptr, next);

  // Tests battery sensitive scheduling parameter.
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[0], next);
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::NOT_CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(nullptr, next);

  entries_[0].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::NOT_CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[0], next);
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[0], next);
}

// Ensures higher priority will be scheduled first.
TEST_F(DownloadSchedulerImplTest, Priority) {
  BuildScheduler(std::vector<int>{0});

  // The second entry has higher priority but is created later than the first
  // entry. This ensures priority is checked before the create time.
  BuildDataEntries(2);
  entries_[0].client = static_cast<DownloadClient>(0);
  entries_[0].scheduling_params.priority = SchedulingParams::Priority::LOW;
  entries_[0].create_time += base::TimeDelta::FromMilliseconds(20);
  entries_[1].client = static_cast<DownloadClient>(0);
  entries_[1].scheduling_params.priority = SchedulingParams::Priority::HIGH;
  entries_[1].create_time += base::TimeDelta::FromMilliseconds(40);

  // Download with higher priority should be polled first, even if there is
  // another download created earlier.
  Entry* next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[1], next);

  // Download with non UI priority should be subject to network and battery
  // scheduling parameters. The higher priority one will be ignored because of
  // mismatching battery condition.
  entries_[1].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
  entries_[0].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;

  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::NOT_CHARGING, NetworkStatus::UNMETERED));
  EXPECT_EQ(&entries_[0], next);
}

// Ensures special priority downloads will bypass other condition checks.
TEST_F(DownloadSchedulerImplTest, UIPriorityIgnoresOtherCriteria) {
  BuildScheduler(std::vector<int>{0});

  // The second download has special priority, strict device status scheduling
  // parameters, and was created later than the first one.
  BuildDataEntries(2);
  entries_[0].client = static_cast<DownloadClient>(0);
  entries_[0].scheduling_params.priority = SchedulingParams::Priority::LOW;
  entries_[0].create_time += base::TimeDelta::FromMilliseconds(20);
  entries_[1].client = static_cast<DownloadClient>(0);
  entries_[1].scheduling_params.priority = SchedulingParams::Priority::UI;
  entries_[1].create_time += base::TimeDelta::FromMilliseconds(40);
  entries_[1].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
  entries_[1].scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::UNMETERED;

  // However, special priority ignores all other validations. When the network
  // and battery resources are limited, download still happens.
  Entry* next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::NOT_CHARGING, NetworkStatus::METERED));
  EXPECT_EQ(&entries_[1], next);

  // Grants the first download UI priority as well. The result now depends on
  // other condition, like create time.
  entries_[0].scheduling_params.priority = SchedulingParams::Priority::UI;
  next = scheduler_->Next(
      entries(),
      BuildDeviceStatus(BatteryStatus::NOT_CHARGING, NetworkStatus::METERED));
  EXPECT_EQ(&entries_[0], next);
}

// Ensures the reschedule logic works correctly, that we can pass the correct
// criteria to platform task scheduler.
TEST_F(DownloadSchedulerImplTest, Reschedule) {
  InSequence s;

  BuildScheduler(std::vector<int>{0});
  BuildDataEntries(2);
  entries_[0].client = static_cast<DownloadClient>(0);
  entries_[1].client = static_cast<DownloadClient>(0);

  entries_[0].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
  entries_[0].scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::UNMETERED;
  entries_[1].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
  entries_[1].scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::UNMETERED;
  EXPECT_CALL(platform_task_scheduler_, ScheduleDownloadTask(true, true))
      .RetiresOnSaturation();
  scheduler_->Reschedule(entries());

  entries_[0].scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
  EXPECT_CALL(platform_task_scheduler_, ScheduleDownloadTask(false, true))
      .RetiresOnSaturation();
  scheduler_->Reschedule(entries());

  entries_[0].scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::NONE;
  EXPECT_CALL(platform_task_scheduler_, ScheduleDownloadTask(false, false))
      .RetiresOnSaturation();
  scheduler_->Reschedule(entries());
}

}  // namespace
}  // namespace download
