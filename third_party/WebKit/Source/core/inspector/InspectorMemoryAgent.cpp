/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/inspector/InspectorMemoryAgent.h"

#include "platform/wtf/dtoa/utils.h"

namespace blink {

using protocol::Response;

const char* InspectorMemoryAgent::counter_names_[] = {
#define INSTANCE_COUNTER_NAME(name) #name "Counter",
    INSTANCE_COUNTERS_LIST(INSTANCE_COUNTER_NAME)
#undef INSTANCE_COUNTER_NAME
};

InspectorMemoryAgent::~InspectorMemoryAgent() {}

Response InspectorMemoryAgent::getDOMCounters(int* documents,
                                              int* nodes,
                                              int* js_event_listeners) {
  *documents =
      InstanceCounters::CounterValue(InstanceCounters::kDocumentCounter);
  *nodes = InstanceCounters::CounterValue(InstanceCounters::kNodeCounter);
  *js_event_listeners =
      InstanceCounters::CounterValue(InstanceCounters::kJSEventListenerCounter);
  return Response::OK();
}

Response InspectorMemoryAgent::enableCounters(
    std::unique_ptr<protocol::Array<protocol::String>> counters) {
  for (size_t i = 0, c = counters->length(); i < c; ++i) {
    auto it = counter_names_map_.find(counters->get(i));
    if (it != counter_names_map_.end())
      enabled_counters_.insert(it->second);
  }
  return Response::OK();
}

Response InspectorMemoryAgent::disableCounters(
    std::unique_ptr<protocol::Array<protocol::String>> counters) {
  for (size_t i = 0, c = counters->length(); i < c; ++i) {
    auto it = counter_names_map_.find(counters->get(i));
    if (it != counter_names_map_.end())
      enabled_counters_.erase(it->second);
  }
  return Response::OK();
}

Response InspectorMemoryAgent::pollCounters(
    std::unique_ptr<protocol::Memory::Counters>* out_result) {
  std::unique_ptr<protocol::DictionaryValue> result =
      protocol::DictionaryValue::create();
  for (auto it : enabled_counters_)
    result->setInteger(counter_names_[it], InstanceCounters::CounterValue(it));
  *out_result = protocol::Object::fromValue(result.get(), nullptr);
  return Response::OK();
}

InspectorMemoryAgent::InspectorMemoryAgent() {
  for (size_t i = 0; i < ARRAY_SIZE(counter_names_); ++i) {
    counter_names_map_.insert(std::make_pair(
        counter_names_[i], static_cast<InstanceCounters::CounterType>(i)));
  }
}

}  // namespace blink
