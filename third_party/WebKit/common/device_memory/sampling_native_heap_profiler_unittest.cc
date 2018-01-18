// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/device_memory/sampling_native_heap_profiler.h"

#include "base/debug/stack_trace.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

class SamplingNativeHeapProfilerTest : public ::testing::Test {};

TEST_F(SamplingNativeHeapProfilerTest, CollectProfile) {
  SamplingNativeHeapProfiler* profiler =
      SamplingNativeHeapProfiler::GetInstance();
  profiler->SuppressRandomnessForTest();
  profiler->SetSamplingInterval(256);
  profiler->Start();
  std::unique_ptr<testing::Message> data(new testing::Message());
  profiler->Stop();
  std::vector<SamplingNativeHeapProfiler::Sample> samples =
      profiler->GetSamples();

  size_t allocation_size = 0;
  for (auto it = samples.begin(); it != samples.end(); ++it) {
    base::debug::StackTrace stack(it->stack.data(), it->stack.size());
    if (stack.ToString().find("blink::(anonymous namespace)::"
                              "SamplingNativeHeapProfilerTest_"
                              "CollectProfile_Test::TestBody()") !=
        std::string::npos) {
      allocation_size += it->size * it->count;
    }
  }
  CHECK_LE(sizeof(testing::Message), allocation_size);
}

}  // namespace
}  // namespace blink
