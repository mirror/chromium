// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/entry_utils.h"

#include "components/download/internal/entry.h"
#include "components/download/public/download_metadata.h"

namespace download {
namespace util {

uint32_t GetNumberOfEntriesForClient(DownloadClient client,
                                     const std::vector<Entry*>& entries) {
  uint32_t count = 0;
  for (auto* entry : entries)
    if (entry->client == client)
      count++;

  return count;
}

std::map<DownloadClient, std::vector<DownloadMetaData>> MapEntriesToClients(
    const std::set<DownloadClient>& clients,
    const std::vector<Entry*>& entries,
    const std::set<Entry::State>& ignored_states) {
  std::map<DownloadClient, std::vector<DownloadMetaData>> categorized;

  for (auto* entry : entries) {
    if (ignored_states.find(entry->state) != ignored_states.end())
      continue;

    DownloadClient client = entry->client;
    if (clients.find(client) == clients.end())
      client = DownloadClient::INVALID;

    categorized[client].push_back(BuildDownloadMetaData(entry));
  }

  return categorized;
}

Criteria GetSchedulingCriteria(const Model::EntryList& entries) {
  Criteria criteria;
  for (auto* const entry : entries) {
    DCHECK(entry);
    const SchedulingParams& scheduling_params = entry->scheduling_params;
    if (scheduling_params.battery_requirements ==
        SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE) {
      criteria.requires_battery_charging = false;
    }
    if (scheduling_params.network_requirements ==
        SchedulingParams::NetworkRequirements::NONE) {
      criteria.requires_unmetered_network = false;
    }
  }
  return criteria;
}

bool EntryBetterThan(const Entry& lhs, const Entry& rhs) {
  return lhs.scheduling_params.priority > rhs.scheduling_params.priority ||
         (lhs.scheduling_params.priority == rhs.scheduling_params.priority &&
          lhs.scheduling_params.cancel_time <
              rhs.scheduling_params.cancel_time);
}

DownloadMetaData BuildDownloadMetaData(Entry* entry) {
  DCHECK(entry);
  DownloadMetaData meta_data;
  meta_data.guid = entry->guid;
  meta_data.path = entry->target_file_path;
  meta_data.completed = entry->state == Entry::State::COMPLETE;
  return meta_data;
}

}  // namespace util
}  // namespace download
