// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_advertisement_device_queue.h"

#include <algorithm>

#include "base/logging.h"
#include "chromeos/components/tether/ble_constants.h"

namespace chromeos {

namespace tether {

namespace {

bool ContainedInVector(
    const ConnectionPriority& connection_priority,
    const cryptauth::RemoteDevice& remote_device,
    const std::vector<BleAdvertisementDeviceQueue::PrioritizedDevice>&
        prioritized_devices) {
  for (const auto& prioritized_device : prioritized_devices) {
    if (prioritized_device.connection_priority == connection_priority &&
        prioritized_device.remote_device == remote_device) {
      return true;
    }
  }

  return false;
}

}  // namespace

BleAdvertisementDeviceQueue::PrioritizedDevice::PrioritizedDevice(
    const cryptauth::RemoteDevice& remote_device,
    const ConnectionPriority& connection_priority)
    : remote_device(remote_device), connection_priority(connection_priority) {}

BleAdvertisementDeviceQueue::PrioritizedDevice::~PrioritizedDevice() {}

BleAdvertisementDeviceQueue::BleAdvertisementDeviceQueue() {}

BleAdvertisementDeviceQueue::~BleAdvertisementDeviceQueue() {}

bool BleAdvertisementDeviceQueue::SetDevices(
    const std::vector<PrioritizedDevice>& devices) {
  bool updated = false;

  // For each device provided, check to see if the device is already part of the
  // queue. If it is not, add it to the end of the deque associated with the
  // device's priority.
  for (const auto& device : devices) {
    std::deque<cryptauth::RemoteDevice>& deque_for_priority =
        priority_to_deque_map_[device.connection_priority];
    if (std::find(deque_for_priority.begin(), deque_for_priority.end(),
                  device.remote_device) == deque_for_priority.end()) {
      deque_for_priority.push_back(device.remote_device);
      updated = true;
    }
  }

  // Now, iterate through each priority's deque to see if any of the entries
  // were not provided as part of the |devices| parameter.
  for (auto& map_entry : priority_to_deque_map_) {
    auto it = map_entry.second.begin();
    while (it != map_entry.second.end()) {
      if (ContainedInVector(map_entry.first, *it, devices)) {
        ++it;
        continue;
      }

      it = map_entry.second.erase(it);
      updated = true;
    }
  }

  return updated;
}

void BleAdvertisementDeviceQueue::MoveDeviceToEnd(
    const std::string& device_id) {
  DCHECK(!device_id.empty());

  for (auto& map_entry : priority_to_deque_map_) {
    auto it = map_entry.second.begin();
    while (it != map_entry.second.end()) {
      if (it->GetDeviceId() == device_id)
        break;
      ++it;
    }

    if (it == map_entry.second.end())
      continue;

    cryptauth::RemoteDevice to_move = *it;
    map_entry.second.erase(it);
    map_entry.second.push_back(to_move);
    return;
  }
}

std::vector<cryptauth::RemoteDevice>
BleAdvertisementDeviceQueue::GetDevicesToWhichToAdvertise() const {
  std::vector<cryptauth::RemoteDevice> devices;
  AddDevicesToVectorForPriority(ConnectionPriority::CONNECTION_PRIORITY_HIGH,
                                &devices);
  AddDevicesToVectorForPriority(ConnectionPriority::CONNECTION_PRIORITY_MEDIUM,
                                &devices);
  AddDevicesToVectorForPriority(ConnectionPriority::CONNECTION_PRIORITY_LOW,
                                &devices);
  DCHECK(devices.size() <= kMaxConcurrentAdvertisements);
  return devices;
}

size_t BleAdvertisementDeviceQueue::GetSize() const {
  size_t count = 0;
  for (const auto& map_entry : priority_to_deque_map_)
    count += map_entry.second.size();
  return count;
}

void BleAdvertisementDeviceQueue::AddDevicesToVectorForPriority(
    ConnectionPriority connection_priority,
    std::vector<cryptauth::RemoteDevice>* output) const {
  if (priority_to_deque_map_.find(connection_priority) ==
      priority_to_deque_map_.end()) {
    // Nothing to do if there is no entry for this priority.
    return;
  }

  const std::deque<cryptauth::RemoteDevice>& deque_for_priority =
      priority_to_deque_map_.at(connection_priority);
  size_t i = 0;
  while (i < deque_for_priority.size() &&
         output->size() < kMaxConcurrentAdvertisements) {
    output->push_back(deque_for_priority[i]);
    ++i;
  }
}

}  // namespace tether

}  // namespace chromeos
