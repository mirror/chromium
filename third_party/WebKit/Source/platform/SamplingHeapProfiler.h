// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SamplingHeapProfiler_h
#define SamplingHeapProfiler_h

#include <vector>

#include "platform/PlatformExport.h"

namespace blink {

class PLATFORM_EXPORT SamplingHeapProfiler {
 public:
  struct Sample {
    size_t size;
    uint32_t count;
    std::vector<void*> stack;
  };

  static uint32_t Start();
  static void Stop();
  static void SetSamplingInterval(size_t interval);
  static void SuppressRandomnessForTest();

  static std::vector<Sample> GetSamples(uint32_t profile_id);
};

}  // namespace blink

#endif  // SamplingHeapProfiler_h
