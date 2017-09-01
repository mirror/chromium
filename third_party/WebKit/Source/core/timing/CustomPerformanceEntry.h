// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CustomPerformanceEntry_h
#define CustomPerformanceEntry_h

#include "core/timing/PerformanceEntry.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CORE_EXPORT CustomPerformanceEntry final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static CustomPerformanceEntry* Create(double start_time,
                                        double end_time,
                                        String name);

  DECLARE_VIRTUAL_TRACE();

 private:
  CustomPerformanceEntry(double start_time, double end_time, String name);
  ~CustomPerformanceEntry() override;
};

}  // namespace blink

#endif  // CustomPerformanceEntry_h
