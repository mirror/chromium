// Copyright 2007-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

#include "counters.h"
#include "platform.h"

namespace v8 { namespace internal {

CounterLookupCallback StatsTable::lookup_function_ = NULL;

StatsCounterTimer::StatsCounterTimer(const wchar_t* name)
  : start_time_(0),  // initialize to avoid compiler complaints
    stop_time_(0) {  // initialize to avoid compiler complaints
  int len = wcslen(name);
  // we prepend the name with 'c.' to indicate that it is a counter.
  name_ = NewArray<wchar_t>(len+3);
  wcscpy(name_, L"t:");
  wcscpy(&name_[2], name);
}

// Start the timer.
void StatsCounterTimer::Start() {
  if (!Enabled())
    return;
  stop_time_ = 0;
  start_time_ = OS::Ticks();
}

// Stop the timer and record the results.
void StatsCounterTimer::Stop() {
  if (!Enabled())
    return;
  stop_time_ = OS::Ticks();
  Record();
}

} }  // namespace v8::internal
