// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cygprofile/lightweight_cygprofile.h"

#include <thread>

#include "base/android/library_loader/anchor_functions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace cygprofile {

namespace {

// Records |addresses_count| distinct addresses |iterations| times, in
// |threads|.
void RecordAddressCost(int iterations,
                       size_t addresses_count,
                       int threads = 1) {
  auto iterate = [iterations, addresses_count]() {
    for (int i = 0; i < iterations; i++) {
      for (size_t addr = kStartOfTextForTesting; addr < addresses_count;
           addr += sizeof(int)) {
        RecordAddressForTesting(addr);
      }
    }
  };
  if (threads != 1) {
    for (int i = 0; i < threads - 1; ++i)
      std::thread(iterate).detach();
  }
  auto tick = base::TimeTicks::Now();
  iterate();
  auto tock = base::TimeTicks::Now();
  double nanos = static_cast<double>((tock - tick).InNanoseconds());
  auto ns_per_call =
      nanos / (iterations * static_cast<double>(addresses_count));
  auto modifier =
      base::StringPrintf("_%d_%d_%d", iterations, addresses_count, threads);
  perf_test::PrintResult("AddEntryCostPerCall", modifier, "", ns_per_call, "ns",
                         true);
}

}  // namespace

class LightweightCygprofileTest : public ::testing::Test {
 protected:
  void SetUp() override { ResetForTesting(); }
};

TEST_F(LightweightCygprofileTest, RecordOffset) {
  size_t first = 1234, second = 1456;
  RecordAddressForTesting(first);
  RecordAddressForTesting(second);
  RecordAddressForTesting(first);      // No duplicates.
  RecordAddressForTesting(first + 1);  // 4 bytes granularity.
  Disable();

  auto reached = GetOrderedOffsetsForTesting();
  EXPECT_EQ(2UL, reached.size());
  EXPECT_EQ(first - kStartOfTextForTesting, reached[0]);
  EXPECT_EQ(second - kStartOfTextForTesting, reached[1]);
}

TEST_F(LightweightCygprofileTest, RecordingStops) {
  size_t first = 1234, second = 1456, third = 1789;
  RecordAddressForTesting(first);
  RecordAddressForTesting(second);
  Disable();
  RecordAddressForTesting(third);

  auto reached = GetOrderedOffsetsForTesting();
  ASSERT_EQ(2UL, reached.size());
  ASSERT_EQ(first - kStartOfTextForTesting, reached[0]);
  ASSERT_EQ(second - kStartOfTextForTesting, reached[1]);
}

TEST_F(LightweightCygprofileTest, OutOfBounds) {
  EXPECT_DEATH(RecordAddressForTesting(kEndOfTextForTesting + 100), "");
  EXPECT_DEATH(RecordAddressForTesting(kStartOfTextForTesting - 100), "");
}

TEST(LightweightCygprofilePerfTest, RecordAddress_10_10000) {
  RecordAddressCost(10, 10000);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_100_10000) {
  RecordAddressCost(100, 10000);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_10_100000) {
  RecordAddressCost(10, 100000);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_100_100000) {
  RecordAddressCost(100, 100000);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_1000_100000_2) {
  RecordAddressCost(1000, 100000, 2);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_1000_100000_3) {
  RecordAddressCost(1000, 100000, 3);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_1000_100000_4) {
  RecordAddressCost(1000, 100000, 4);
}

TEST(LightweightCygprofilePerfTest, RecordAddress_1000_100000_6) {
  RecordAddressCost(1000, 100000, 6);
}

}  // namespace cygprofile

// Custom runner implementation since base's one requires JNI on Android.
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
