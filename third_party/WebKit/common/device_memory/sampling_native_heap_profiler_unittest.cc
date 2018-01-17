// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/device_memory/sampling_native_heap_profiler.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

class SamplingNativeHeapProfilerTest : public ::testing::Test {};

TEST_F(SamplingNativeHeapProfilerTest, CollectProfile) {
#if 0
  ApproximatedDeviceMemory::SetPhysicalMemoryMBForTesting(32768);  // 32GB
  EXPECT_EQ(8, ApproximatedDeviceMemory::GetApproximatedDeviceMemory());
  ApproximatedDeviceMemory::SetPhysicalMemoryMBForTesting(64385);  // <64GB
  EXPECT_EQ(8, ApproximatedDeviceMemory::GetApproximatedDeviceMemory());
#endif
}

}  // namespace
}  // namespace blink
