// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/core/common/crash_key.h"

#include "base/debug/stack_trace.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace crash_reporter {
namespace {

class CrashKeyStringTest : public testing::Test {
 public:
  void SetUp() override { InitializeCrashKeys(); }
};

TEST_F(CrashKeyStringTest, ScopedCrashKeyString) {
  static CrashKeyString<32> key("test-scope");

  EXPECT_FALSE(key.is_set());

  {
    ScopedCrashKeyString scoper(&key, "value");
    EXPECT_TRUE(key.is_set());
  }

  EXPECT_FALSE(key.is_set());
}

TEST_F(CrashKeyStringTest, FormatStackTrace) {
  const uint64_t addresses[] = {
      0x0badbeef, 0x7777888899990000, 0xabc, 0x000000ddeeff11, 0x123456789abc,
  };
  base::debug::StackTrace trace(reinterpret_cast<const void* const*>(addresses),
                                arraysize(addresses));

  std::string too_small = internal::FormatStackTrace(trace, 3);
  EXPECT_EQ(0u, too_small.size());

  std::string one_value = internal::FormatStackTrace(trace, 18);
  EXPECT_EQ("0xbadbeef", one_value);

  std::string three_values = internal::FormatStackTrace(trace, 34);
  EXPECT_EQ("0xbadbeef 0x7777888899990000 0xabc", three_values);

  std::string all_values = internal::FormatStackTrace(trace, 256);
  EXPECT_EQ("0xbadbeef 0x7777888899990000 0xabc 0xddeeff11 0x123456789abc",
            all_values);
}

TEST_F(CrashKeyStringTest, SetStackTrace) {
  static CrashKeyString<1024> key("test-trace");

  EXPECT_FALSE(key.is_set());

  SetCrashKeyStringToStackTrace(&key, base::debug::StackTrace());

  EXPECT_TRUE(key.is_set());
}

}  // namespace
}  // namespace crash_reporter
