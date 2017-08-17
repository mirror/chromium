// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PerformanceElementTiming_h
#define PerformanceElementTiming_h

#include "core/CoreExport.h"
#include "core/timing/PerformanceEntry.h"

namespace blink {

class CORE_EXPORT PerformanceElementTiming final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  PerformanceElementTiming(const String& name, double start_time);
  ~PerformanceElementTiming() override;
};
}  // namespace blink

#endif  // PerformanceElementTiming_h
