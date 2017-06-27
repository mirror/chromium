// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/RuntimeCallStats.h"

#include <inttypes.h>
#include <algorithm>
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

namespace {
RuntimeCallStats* g_runtime_call_stats_for_testing = nullptr;
}

void RuntimeCallCounter::Dump(TracedValue& value) {
  value.BeginArray(name_);
  value.PushDouble(count_);
  value.PushDouble(time_.InMicroseconds());
  value.EndArray();
}

void RuntimeCallTimer::Start(RuntimeCallCounter* counter,
                             RuntimeCallTimer* parent) {
  DCHECK(!IsRunning());
  counter_ = counter;
  parent_ = parent;
  start_ticks_ = TimeTicks::Now();
  if (parent_)
    parent_->Pause(start_ticks_);
}

RuntimeCallTimer* RuntimeCallTimer::Stop() {
  DCHECK(IsRunning());
  TimeTicks now = TimeTicks::Now();
  elapsed_time_ += (now - start_ticks_);
  start_ticks_ = TimeTicks();
  counter_->IncrementAndAddTime(elapsed_time_);
  if (parent_)
    parent_->Resume(now);
  return parent_;
}

RuntimeCallStats::RuntimeCallStats() {
  static const char* const names[] = {
#define COUNTER_NAME_ENTRY(name) "Blink_" #name,
      FOR_EACH_COUNTER(COUNTER_NAME_ENTRY)
#undef COUNTER_NAME_ENTRY
  };

  for (int i = 0; i < number_of_counters_; i++) {
    counters_[i] = RuntimeCallCounter(names[i]);
  }
}

// static
RuntimeCallStats* RuntimeCallStats::From(v8::Isolate* isolate) {
  if (g_runtime_call_stats_for_testing)
    return g_runtime_call_stats_for_testing;
  return V8PerIsolateData::From(isolate)->GetRuntimeCallStats();
}

void RuntimeCallStats::Reset() {
  for (int i = 0; i < number_of_counters_; i++) {
    counters_[i].Reset();
  }
  in_use_ = true;
}

void RuntimeCallStats::Dump(TracedValue& value) {
  for (int i = 0; i < number_of_counters_; i++) {
    if (counters_[i].GetCount() > 0)
      counters_[i].Dump(value);
  }
  in_use_ = false;
}

String RuntimeCallStats::ToString() const {
  StringBuilder builder;
  builder.Append("Runtime Call Stats for Blink \n");
  builder.Append(
      "Name                                                    Count     Time "
      "(ms)\n\n");
  for (int i = 0; i < number_of_counters_; i++) {
    const RuntimeCallCounter* counter = &counters_[i];
    builder.Append(String::Format("%-55s  %8" PRIu64 "  %9.3f\n",
                                  counter->GetName(), counter->GetCount(),
                                  counter->GetTime().InMillisecondsF()));
  }
  return builder.ToString();
}

// static
void RuntimeCallStats::SetRuntimeCallStatsForTesting() {
  DEFINE_STATIC_LOCAL(RuntimeCallStats, s_rcs_for_testing, ());
  g_runtime_call_stats_for_testing =
      static_cast<RuntimeCallStats*>(&s_rcs_for_testing);
}

// static
void RuntimeCallStats::ClearRuntimeCallStatsForTesting() {
  g_runtime_call_stats_for_testing = nullptr;
}

void RuntimeCallStatsScopedTracer::InitializeData(RuntimeCallStats* stats,
                                                  const char* category_group,
                                                  const char* name) {
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(category_group, &category_group_enabled_);
  data_.category_group = category_group;
  data_.name = name;
  if (ShouldTrace()) {
    if (stats->InUse())
      has_parent_scope_ = true;
    else
      stats->Reset();
    data_.stats = stats;
  }
}

void RuntimeCallStatsScopedTracer::AddEndTraceEvent() {
  if (ShouldTrace() && !has_parent_scope_) {
    std::unique_ptr<TracedValue> value = TracedValue::Create();
    data_.stats->Dump(*value);
    TRACE_EVENT_END1(data_.category_group, data_.name, "runtime-call-stats",
                     std::move(value));
  } else if (begin_event_created_) {
    TRACE_EVENT_END0(data_.category_group, data_.name);
  }
}

}  // namespace blink
