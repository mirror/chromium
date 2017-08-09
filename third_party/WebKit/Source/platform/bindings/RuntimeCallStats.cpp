// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/RuntimeCallStats.h"

#include <inttypes.h>
#include <algorithm>
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/wtf/text/StringBuilder.h"
#include "public/web/WebKit.h"

namespace blink {

// Wrapper function defined in WebKit.h
void LogRuntimeCallStats() {
  LOG(INFO)
      << "\n"
      << RuntimeCallStats::From(MainThreadIsolate())->ToString().Utf8().data();
}

namespace {
RuntimeCallStats* g_runtime_call_stats_for_testing = nullptr;
}

void RuntimeCallCounter::Dump(TracedValue& value) const {
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
  for (RuntimeCallCounter* counter : CounterMapToSortedArray()) {
    counter->Reset();
  }
}

void RuntimeCallStats::Dump(TracedValue& value) const {
  for (int i = 0; i < number_of_counters_; i++) {
    if (counters_[i].GetCount() > 0)
      counters_[i].Dump(value);
  }
  for (const RuntimeCallCounter* counter : CounterMapToSortedConstArray()) {
    if (counter->GetCount() > 0)
      counter->Dump(value);
  }
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
  AddCounterMapStatsToBuilder(builder);
  return builder.ToString();
}

Vector<const RuntimeCallCounter*>
RuntimeCallStats::CounterMapToSortedConstArray() const {
  Vector<const RuntimeCallCounter*> counters;
  for (const std::unique_ptr<RuntimeCallCounter>& counter :
       counter_map_.Values()) {
    counters.push_back(counter.get());
  }
  auto comparator = [](const RuntimeCallCounter* a,
                       const RuntimeCallCounter* b) {
    return a->GetCount() == b->GetCount()
               ? strcmp(a->GetName(), b->GetName()) < 0
               : a->GetCount() < b->GetCount();
  };
  std::sort(counters.begin(), counters.end(), comparator);
  return counters;
}

Vector<RuntimeCallCounter*> RuntimeCallStats::CounterMapToSortedArray() const {
  Vector<RuntimeCallCounter*> counters;
  for (const std::unique_ptr<RuntimeCallCounter>& counter :
       counter_map_.Values()) {
    counters.push_back(counter.get());
  }
  auto comparator = [](RuntimeCallCounter* a, RuntimeCallCounter* b) {
    return a->GetCount() == b->GetCount()
               ? strcmp(a->GetName(), b->GetName()) < 0
               : a->GetCount() < b->GetCount();
  };
  std::sort(counters.begin(), counters.end(), comparator);
  return counters;
}

void RuntimeCallStats::AddCounterMapStatsToBuilder(
    StringBuilder& builder) const {
  builder.Append(String::Format("\nNumber of counters in map: %u\n\n",
                                counter_map_.size()));
  for (const RuntimeCallCounter* counter : CounterMapToSortedConstArray()) {
    builder.Append(String::Format("%-50s  %8" PRIu64 "  %9.3f\n",
                                  counter->GetName(), counter->GetCount(),
                                  counter->GetTime().InMillisecondsF()));
  }
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

const char* const RuntimeCallStatsScopedTracer::s_category_group_ =
    TRACE_DISABLED_BY_DEFAULT("v8.runtime_stats");
const char* const RuntimeCallStatsScopedTracer::s_name_ =
    "BlinkRuntimeCallStats";

void RuntimeCallStatsScopedTracer::AddBeginTraceEvent() {
  stats_->Reset();
  stats_->SetInUse(true);
  TRACE_EVENT_BEGIN0(s_category_group_, s_name_);
}

void RuntimeCallStatsScopedTracer::AddEndTraceEvent() {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  stats_->Dump(*value);
  stats_->SetInUse(false);
  TRACE_EVENT_END1(s_category_group_, s_name_, "runtime-call-stats",
                   std::move(value));
}

}  // namespace blink
