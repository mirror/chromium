// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/CustomPerformanceEntry.h"

namespace blink {

// static
CustomPerformanceEntry* CustomPerformanceEntry::Create(double start_time,
                                                       double end_time,
                                                       String name) {
  return new CustomPerformanceEntry(start_time, end_time, name);
}

CustomPerformanceEntry::CustomPerformanceEntry(double start_time,
                                               double end_time,
                                               String name)
    : PerformanceEntry(name, "custom", start_time, end_time) {}

CustomPerformanceEntry::~CustomPerformanceEntry() {}

DEFINE_TRACE(CustomPerformanceEntry) {
  PerformanceEntry::Trace(visitor);
}

}  // namespace blink
