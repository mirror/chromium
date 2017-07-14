// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/entry.h"

namespace download {

Entry::Entry() : attempt_count(0) {}
Entry::Entry(const Entry& other)
    : client(other.client),
      guid(other.guid),
      create_time(other.create_time),
      scheduling_params(other.scheduling_params),
      request_params(other.request_params),
      state(other.state),
      target_file_path(other.target_file_path),
      completion_time(other.completion_time),
      attempt_count(other.attempt_count) {
  if (other.has_traffic_annotation())
    set_traffic_annotation(other.get_traffic_annotation());
  else
    reset_traffic_annotation();
}

Entry::Entry(const DownloadParams& params)
    : client(params.client),
      guid(params.guid),
      create_time(base::Time::Now()),
      scheduling_params(params.scheduling_params),
      request_params(params.request_params),
      attempt_count(0) {
  set_traffic_annotation(params.traffic_annotation_);
}

Entry::~Entry() = default;

bool Entry::operator==(const Entry& other) const {
  return client == other.client && guid == other.guid &&
         scheduling_params.cancel_time == other.scheduling_params.cancel_time &&
         scheduling_params.network_requirements ==
             other.scheduling_params.network_requirements &&
         scheduling_params.battery_requirements ==
             other.scheduling_params.battery_requirements &&
         scheduling_params.priority == other.scheduling_params.priority &&
         request_params.url == other.request_params.url &&
         request_params.method == other.request_params.method &&
         request_params.request_headers.ToString() ==
             other.request_params.request_headers.ToString() &&
         state == other.state && target_file_path == other.target_file_path &&
         create_time == other.create_time &&
         completion_time == other.completion_time &&
         attempt_count == other.attempt_count &&
         (has_traffic_annotation() == other.has_traffic_annotation() &&
          (!has_traffic_annotation() ||
           get_traffic_annotation() == other.get_traffic_annotation()));
}

Entry& Entry::operator=(const Entry& other) {
  client = other.client;
  guid = other.guid;
  create_time = other.create_time;
  scheduling_params = other.scheduling_params;
  request_params = other.request_params;
  state = other.state;
  target_file_path = other.target_file_path;
  completion_time = other.completion_time;
  attempt_count = other.attempt_count;
  if (other.has_traffic_annotation())
    set_traffic_annotation(other.get_traffic_annotation());
  else
    reset_traffic_annotation();

  return *this;
}
}  // namespace download
