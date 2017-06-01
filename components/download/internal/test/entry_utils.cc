// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "components/download/internal/test/entry_utils.h"

namespace download {
namespace test {

bool CompareEntry(const Entry* const& expected, const Entry* const& actual) {
  if (expected == nullptr || actual == nullptr)
    return expected == actual;

  return *expected == *actual;
}

bool CompareEntryList(const std::vector<Entry*>& expected,
                      const std::vector<Entry*>& actual) {
  return std::is_permutation(actual.cbegin(), actual.cend(), expected.cbegin(),
                             CompareEntry);
}

bool CompareEntryList(const std::vector<Entry>& list1,
                      const std::vector<Entry>& list2) {
  return std::is_permutation(list1.begin(), list1.end(), list2.begin());
}

Entry BuildEntry(DownloadClient client, const std::string& guid) {
  Entry entry;
  entry.client = client;
  entry.guid = guid;
  return entry;
}

Entry BuildEntry(DownloadClient client,
                 const std::string& guid,
                 base::Time cancel_time,
                 SchedulingParams::NetworkRequirements network_requirements,
                 SchedulingParams::BatteryRequirements battery_requirements,
                 SchedulingParams::Priority priority,
                 const GURL& url,
                 const std::string& request_method,
                 Entry::State state) {
  Entry entry = BuildEntry(client, guid);
  entry.scheduling_params.cancel_time = cancel_time;
  entry.scheduling_params.network_requirements = network_requirements;
  entry.scheduling_params.battery_requirements = battery_requirements;
  entry.scheduling_params.priority = priority;
  entry.request_params.url = url;
  entry.request_params.method = request_method;
  entry.state = state;
  return entry;
}

}  // namespace test
}  // namespace download
