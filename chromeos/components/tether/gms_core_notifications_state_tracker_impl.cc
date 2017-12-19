// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/gms_core_notifications_state_tracker_impl.h"

namespace chromeos {

namespace tether {

GmsCoreNotificationsStateTrackerImpl::GmsCoreNotificationsStateTrackerImpl() =
    default;

GmsCoreNotificationsStateTrackerImpl::~GmsCoreNotificationsStateTrackerImpl() {
  bool was_empty = device_names_.empty();
  device_names_.clear();

  if (!was_empty)
    NotifyGmsCoreNotificationStateChanged();
}

const std::unordered_set<std::string>& GmsCoreNotificationsStateTrackerImpl::
    GetGmsCoreNotificationsDisabledDeviceNames() {
  return device_names_;
}

void GmsCoreNotificationsStateTrackerImpl::OnTetherAvailabilityResponse(
    const std::vector<HostScannerOperation::ScannedDeviceInfo>&
        scanned_device_list_so_far,
    const std::vector<std::string>&
        gms_core_notifications_disabled_device_names,
    bool is_final_scan_result) {
  // Insert all names gathered by this scan to |device_names_|.
  size_t old_size = device_names_.size();
  for (const auto& device_name : gms_core_notifications_disabled_device_names)
    device_names_.insert(device_name);

  // Because |device_names_| is a set, its elements are unique; thus, any change
  // in the set implies a change in the set's size.
  bool names_changed = old_size != device_names_.size();

  // If this is the final scan result, any names in |device_names_| which are
  // not also present in |gms_core_notifications_disabled_device_names| were
  // added in a previous host scan. Since this is the last update for the
  // current scan, these leftover names which are no longer present represent
  // stale data which should be removed.
  if (is_final_scan_result) {
    auto it = device_names_.begin();
    while (it != device_names_.end()) {
      if (!base::ContainsValue(gms_core_notifications_disabled_device_names,
                               *it)) {
        it = device_names_.erase(it);
        names_changed = true;
      } else {
        ++it;
      }
    }
  }

  if (names_changed)
    NotifyGmsCoreNotificationStateChanged();
}

}  // namespace tether

}  // namespace chromeos
