// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/SamplingHeapProfiler.h"

#include "base/heap_profiler/sampling_heap_profiler.h"

namespace blink {

uint32_t SamplingHeapProfiler::Start() {
  return base::heap_profiler::SamplingHeapProfiler::GetInstance()->Start();
}

void SamplingHeapProfiler::Stop() {
  base::heap_profiler::SamplingHeapProfiler::GetInstance()->Stop();
}

void SamplingHeapProfiler::SetSamplingInterval(size_t interval) {
  base::heap_profiler::SamplingHeapProfiler::GetInstance()->SetSamplingInterval(
      interval);
}

void SamplingHeapProfiler::SuppressRandomnessForTest() {
  base::heap_profiler::SamplingHeapProfiler::GetInstance()
      ->SuppressRandomnessForTest();
}

std::vector<SamplingHeapProfiler::Sample> SamplingHeapProfiler::GetSamples(
    uint32_t profile_id) {
  std::vector<SamplingHeapProfiler::Sample> result;
  std::vector<base::heap_profiler::SamplingHeapProfiler::Sample> samples =
      base::heap_profiler::SamplingHeapProfiler::GetInstance()->GetSamples(
          profile_id);
  for (auto& sample : samples)
    result.push_back({sample.size, sample.count, std::move(sample.stack)});
  return result;
}

}  // namespace blink
