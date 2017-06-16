// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_number_conversions.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace base {

namespace {
template <typename INT>
void RunTest(INT start, INT end, std::string (*func)(INT)) {
  TimeTicks start_time = TimeTicks::Now();
  while (start <= end) {
    func(start++);
  }
  TimeTicks end_time = TimeTicks::Now();
  perf_test::PrintResult("IntToString", "", "",
                         (end_time - start_time).InMillisecondsF(), "ms", true);
}

void RunTest(size_t start, size_t end, std::string (*func)(size_t)) {
  TimeTicks start_time = TimeTicks::Now();
  while (start <= end) {
    func(start++);
  }
  TimeTicks end_time = TimeTicks::Now();
  perf_test::PrintResult("IntToString", "", "",
                         (end_time - start_time).InMillisecondsF(), "ms", true);
}

}  // namespace

TEST(StringNumberConversionsPerfTest, IntToString_PositiveSmall) {
  RunTest(0, 1000, &IntToString);
}

TEST(StringNumberConversionsPerfTest, IntToString_NegativeSmall) {
  RunTest(-1000, 0, &IntToString);
}

TEST(StringNumberConversionsPerfTest, IntToString_PositiveLarge) {
  RunTest(100000, 1000000, &IntToString);
}

TEST(StringNumberConversionsPerfTest, IntToString_NegativeLarge) {
  RunTest(-1000000, 100000, &IntToString);
}

TEST(StringNumberConversionsPerfTest, SizeTToString_PositiveSmall) {
  RunTest(0u, 1000u, &SizeTToString);
}

TEST(StringNumberConversionsPerfTest, SizeTToString_PositiveLarge) {
  RunTest(100000u, 1000000u, &SizeTToString);
}

}  // namespace base
