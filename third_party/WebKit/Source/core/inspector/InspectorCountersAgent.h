// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InspectorCountersAgent_h
#define InspectorCountersAgent_h

#include <unordered_map>
#include <unordered_set>
#include "core/CoreExport.h"
#include "core/inspector/InspectorBaseAgent.h"
#include "core/inspector/protocol/Counters.h"
#include "platform/InstanceCounters.h"

namespace blink {

class CORE_EXPORT InspectorCountersAgent final
    : public InspectorBaseAgent<protocol::Counters::Metainfo> {
  WTF_MAKE_NONCOPYABLE(InspectorCountersAgent);

 public:
  static InspectorCountersAgent* Create() {
    return new InspectorCountersAgent();
  }
  ~InspectorCountersAgent() override;

  protocol::Response enableCounters(
      std::unique_ptr<protocol::Array<protocol::String>> counters) override;
  protocol::Response getCounters(
      std::unique_ptr<protocol::Array<protocol::Counters::Counter>>* out_result)
      override;

 private:
  InspectorCountersAgent();

  std::unordered_set<InstanceCounters::CounterType> enabled_counters_;
  std::unordered_map<String, InstanceCounters::CounterType> counter_names_map_;
  static const char* counter_names_[];
};

}  // namespace blink

#endif  // !defined(InspectorCountersAgent_h)
