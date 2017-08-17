// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/PerformanceElementTiming.h"

namespace blink {

PerformanceElementTiming::PerformanceElementTiming(const String& name,
                                                   double start_time)
    : PerformanceEntry(name, "elementtiming", start_time, start_time) {}

PerformanceElementTiming::~PerformanceElementTiming() {}

}  // namespace blink
