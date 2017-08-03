// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorCountersAgent.h"

#include "platform/wtf/dtoa/utils.h"

namespace blink {

using protocol::Response;

const char* InspectorCountersAgent::counter_names_[] = {
#define INSTANCE_COUNTER_NAME(name) #name "Counter",
    INSTANCE_COUNTERS_LIST(INSTANCE_COUNTER_NAME)
#undef INSTANCE_COUNTER_NAME
};

InspectorCountersAgent::InspectorCountersAgent() {
  for (size_t i = 0; i < ARRAY_SIZE(counter_names_); ++i) {
    counter_names_map_.insert(std::make_pair(
        counter_names_[i], static_cast<InstanceCounters::CounterType>(i)));
  }
}

InspectorCountersAgent::~InspectorCountersAgent() = default;

Response InspectorCountersAgent::enableCounters(
    std::unique_ptr<protocol::Array<protocol::String>> counters) {
  enabled_counters_.clear();
  for (size_t i = 0, c = counters->length(); i < c; ++i) {
    auto it = counter_names_map_.find(counters->get(i));
    if (it != counter_names_map_.end())
      enabled_counters_.insert(it->second);
  }
  return Response::OK();
}

Response InspectorCountersAgent::getCounters(
    std::unique_ptr<protocol::Array<protocol::Counters::Counter>>* out_result) {
  std::unique_ptr<protocol::Array<protocol::Counters::Counter>> result =
      protocol::Array<protocol::Counters::Counter>::create();
  for (auto it : enabled_counters_) {
    result->addItem(protocol::Counters::Counter::create()
                        .setName(counter_names_[it])
                        .setValue(InstanceCounters::CounterValue(it))
                        .build());
  }
  *out_result = std::move(result);
  return Response::OK();
}

}  // namespace blink
