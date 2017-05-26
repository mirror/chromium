// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/RuntimeCallStats.h"

#include <algorithm>
#include <iomanip>

namespace blink {

const RuntimeCallStats::CounterId RuntimeCallStats::counters_[] = {
#define COUNTER_ENTRY(name) &RuntimeCallStats::name,
    FOR_EACH_COUNTER(COUNTER_ENTRY)
#undef COUNTER_ENTRY
};

const int RuntimeCallStats::number_of_counters_ =
    arraysize(RuntimeCallStats::counters_);

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
#define INIT_COUNTER(name) this->name = RuntimeCallCounter(#name);
  FOR_EACH_COUNTER(INIT_COUNTER)
#undef INIT_COUNTER
}

void RuntimeCallStats::Reset() {
  for (int i = 0; i < number_of_counters_; i++) {
    (this->*counters_[i]).Reset();
  }
}

size_t RuntimeCallStats::maxCounterNameLength() const {
  auto comparator = [this](CounterId a, CounterId b) {
    return strlen((this->*a).GetName()) < strlen((this->*b).GetName());
  };
  CounterId max =
      *std::max_element(counters_, counters_ + number_of_counters_, comparator);
  return strlen((this->*max).GetName());
}

std::ostream& operator<<(std::ostream& out, const RuntimeCallStats& stats) {
  int space_between_cols = 2;
  int name_col_width = stats.maxCounterNameLength() + space_between_cols;
  int max_num_of_digits = 10;
  int count_col_width = max_num_of_digits + space_between_cols;

  out.precision(3);
  out << "Runtime Call Stats for Blink\n\n";
  out << std::left << std::setfill(' ');

  out << std::setw(name_col_width) << "Name";
  out << std::setw(count_col_width) << "Count";
  out << "Time (msec)\n\n";

  for (int i = 0; i < stats.number_of_counters_; i++) {
    RuntimeCallCounter counter = stats.*(stats.counters_[i]);
    out << std::setw(name_col_width) << counter.GetName();
    out << std::setw(count_col_width) << counter.GetCount();
    out << static_cast<double>(counter.GetTime().InMicroseconds()) / 1000;
    out << "\n";
  }

  out << std::endl;

  return out;
}

}  // namespace blink
