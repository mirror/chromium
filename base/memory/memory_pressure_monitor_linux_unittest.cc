// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/memory_pressure_monitor_linux.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class TestMemoryPressureMonitor : public MemoryPressureMonitorLinux {
 public:
  // A HistogramTester for verifying correct UMA stat generation.
  base::HistogramTester tester;

  TestMemoryPressureMonitor() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestMemoryPressureMonitor);
};

TEST(LinuxMemoryPressureMonitorTest, MemoryPressureFromLinuxMemoryPressure) {}

TEST(LinuxMemoryPressureMonitorTest, CurrentMemoryPressure) {
  TestMemoryPressureMonitor monitor;

  MemoryPressureListener::MemoryPressureLevel memory_pressure =
      monitor.GetCurrentPressureLevel();
  EXPECT_TRUE(memory_pressure ==
                  MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE ||
              memory_pressure ==
                  MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE ||
              memory_pressure ==
                  MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
}

TEST(LinuxMemoryPressureMonitorTest, MemoryPressureConversion) {}

TEST(LinuxMemoryPressureMonitorTest, MemoryPressureRunLoopChecking) {}

TEST(LinuxMemoryPressureMonitorTest, RecordMemoryPressureStats) {}

}  // namespace base
