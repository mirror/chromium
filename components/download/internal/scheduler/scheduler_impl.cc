// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/scheduler/scheduler_impl.h"

#include <map>

#include "components/download/internal/scheduler/device_status.h"
#include "components/download/public/download_params.h"

namespace download {

SchedulerImpl::SchedulerImpl(PlatformTaskScheduler* platform_scheduler,
                             const std::vector<int>& clients)
    : platform_scheduler_(platform_scheduler),
      download_clients_(clients),
      current_client_index_(0) {
  DCHECK(!clients.empty());
}

SchedulerImpl::~SchedulerImpl() {}

void SchedulerImpl::Reschedule(const Model::EntryList& entries) {
  if (entries.empty())
    return;

  bool require_charging = true;
  bool require_unmetered_network = true;

  for (auto* const entry : entries) {
    DCHECK(entry);
    const SchedulingParams& scheduling_params = entry->scheduling_params;
    if (scheduling_params.battery_requirements ==
        SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE) {
      require_charging = false;
    }
    // TODO(xingliu): Support NetworkRequirements::OPTIMISTIC.
    if (scheduling_params.network_requirements ==
        SchedulingParams::NetworkRequirements::NONE) {
      require_unmetered_network = false;
    }
  }

  if (platform_scheduler_)
    platform_scheduler_->ScheduleDownloadTask(require_charging,
                                              require_unmetered_network);
}

Entry* SchedulerImpl::Next(const Model::EntryList& entries,
                           const DeviceStatus& device_status) {
  Entry* download = nullptr;
  int orginal_client_index = current_client_index_;

  // Maps entries with their client id.
  std::map<int, Model::EntryList> entries_map;
  for (auto* const entry : entries) {
    entries_map[static_cast<int>(entry->client)].push_back(entry);
  }

  // Finds the next download.
  do {
    int client = download_clients_[current_client_index_];
    download = GetNextByClientId(client, entries_map[client], device_status);

    // Round robin load balancing between clients, until we find a download,
    // or has queried entries for all clients.
    current_client_index_ =
        (current_client_index_ + 1) % download_clients_.size();

  } while (!download && current_client_index_ != orginal_client_index);

  return download;
}

Entry* SchedulerImpl::GetNextByClientId(int client,
                                        const Model::EntryList& entries,
                                        const DeviceStatus& device_status) {
  Entry* download = nullptr;
  if (entries.empty())
    return download;

  for (auto* const entry : entries) {
    DCHECK(entry);
    DCHECK_EQ(static_cast<int>(entry->client), client);
    const SchedulingParams& current_params = entry->scheduling_params;

    // Every download needs to pass the state check.
    if (entry->state != Entry::State::AVAILABLE) {
      continue;
    }

    // Filter out downloads based on scheduling parameters.
    // Downloads with Priority::UI will ignore all device states check.
    if (entry->scheduling_params.priority != SchedulingParams::Priority::UI &&
        !(device_status.MeetCondition(current_params).MeetRequirements())) {
      continue;
    }

    // Find the most appropriate download based on priority and create time.
    // The priority check is evaluated before the create time check.
    if (!download ||
        (current_params.priority > download->scheduling_params.priority ||
         entry->create_time < download->create_time)) {
      download = entry;
    }
  }

  return download;
}

}  // namespace download
