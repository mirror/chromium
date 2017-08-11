// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/core/common/crash_keys.h"

#include <stddef.h>

#include <map>
#include <string>

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug/crash_logging.h"
#include "base/format_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

class CrashKeysTest : public testing::Test {
 public:
  void SetUp() override {
    self_ = this;
    base::debug::SetCrashKeyReportingFunctions(
        &SetCrashKeyValue, &ClearCrashKey);
  }

  bool InitSwitchesCrashKeys() {
    std::vector<base::debug::CrashKey> keys = {
        {crash_keys::kNumSwitches, crash_keys::kSmallSize},
        {crash_keys::kSwitches, crash_keys::kGiganticSize}};
    return InitCrashKeys(keys);
  }

  bool InitVariationsCrashKeys() {
    std::vector<base::debug::CrashKey> keys = {
        {crash_keys::kNumVariations, crash_keys::kSmallSize},
        {crash_keys::kVariations, crash_keys::kHugeSize}};
    return InitCrashKeys(keys);
  }

  void TearDown() override {
    base::debug::ResetCrashLoggingForTesting();
    self_ = NULL;
  }

  bool HasCrashKey(const std::string& key) {
    return keys_.find(key) != keys_.end();
  }

  std::string GetKeyValue(const std::string& key) {
    std::map<std::string, std::string>::const_iterator it = keys_.find(key);
    if (it == keys_.end())
      return std::string();
    return it->second;
  }

 private:
  bool InitCrashKeys(const std::vector<base::debug::CrashKey>& keys) {
    base::debug::InitCrashKeys(keys.data(), keys.size(),
                               crash_keys::kChunkMaxLength);
    return !keys.empty();
  }

  static void SetCrashKeyValue(const base::StringPiece& key,
                               const base::StringPiece& value) {
    self_->keys_[key.as_string()] = value.as_string();
  }

  static void ClearCrashKey(const base::StringPiece& key) {
    self_->keys_.erase(key.as_string());
  }

  static CrashKeysTest* self_;

  std::map<std::string, std::string> keys_;
};

CrashKeysTest* CrashKeysTest::self_ = NULL;

namespace {

size_t NumChunksForLength(size_t length) {
  return (length + crash_keys::kChunkMaxLength - 1) /
         crash_keys::kChunkMaxLength;
}

std::string GetSwitchKey(size_t chunk) {
  return base::StringPrintf("%s-%" PRIuS, crash_keys::kSwitches, chunk);
}

}  // namespace

TEST_F(CrashKeysTest, Switches) {
  ASSERT_TRUE(InitSwitchesCrashKeys());

  // Adds three rows with switches.
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    const size_t min_swithes_string_length = crash_keys::kChunkMaxLength * 3;
    ASSERT_GE(crash_keys::kGiganticSize, min_swithes_string_length);
    std::string expected_swithces;
    for (size_t i = 0; expected_swithces.size() < min_swithes_string_length;
         ++i) {
      std::string flag = base::StringPrintf("--flag-%" PRIuS, i);
      expected_swithces.append(flag).append(" ");
      command_line.AppendSwitch(flag);
    }
    size_t chunk_count = NumChunksForLength(expected_swithces.size());
    EXPECT_GE(NumChunksForLength(crash_keys::kGiganticSize), chunk_count);
    crash_keys::SetSwitchesFromCommandLine(command_line, nullptr);
    std::string assembled_switches;
    for (size_t i = 1; i <= chunk_count; ++i)
      assembled_switches.append(GetKeyValue(GetSwitchKey(i)));
    ASSERT_EQ(expected_swithces, assembled_switches);
  }

  // Add swithes longer then max limit.
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    std::string expected_swithces;
    for (size_t i = 0; NumChunksForLength(expected_swithces.size()) <=
                           NumChunksForLength(crash_keys::kGiganticSize);
         ++i) {
      std::string flag = base::StringPrintf("--flag-%" PRIuS, i);
      expected_swithces.append(flag).append(" ");
      command_line.AppendSwitch(flag);
    }
    crash_keys::SetSwitchesFromCommandLine(command_line, nullptr);
    const size_t chunk_count = NumChunksForLength(crash_keys::kGiganticSize);
    ASSERT_TRUE(HasCrashKey(GetSwitchKey(chunk_count)));
    ASSERT_FALSE(HasCrashKey(GetSwitchKey(chunk_count + 1)));

    std::string assembled_switches;
    for (size_t i = 1; i <= chunk_count; ++i)
      assembled_switches.append(GetKeyValue(GetSwitchKey(i)));
    ASSERT_TRUE(expected_swithces.find(assembled_switches) !=
                std::string::npos);
    ASSERT_NE(expected_swithces, assembled_switches);
  }
}

namespace {

bool IsBoringFlag(const std::string& flag) {
  return flag.compare("--boring") == 0;
}

}  // namespace

TEST_F(CrashKeysTest, FilterFlags) {
  ASSERT_TRUE(InitSwitchesCrashKeys());
  std::string expected_swithces;
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitch("--not-boring-1");
  expected_swithces.append("--not-boring-1").append(" ");
  command_line.AppendSwitch("--boring");

  // Include the max number of non-boring switches, to make sure that only the
  // switches actually included in the crash keys are counted.
  for (size_t i = 2; i <= 10; ++i) {
    std::string flag = base::StringPrintf("--not-boring-%" PRIuS, i);
    expected_swithces.append(flag).append(" ");
    command_line.AppendSwitch(flag);
  }
  crash_keys::SetSwitchesFromCommandLine(command_line, &IsBoringFlag);

  size_t chunk_count = NumChunksForLength(expected_swithces.size());
  std::string assembled_swithces;
  for (size_t i = 1; i <= chunk_count; ++i)
    assembled_swithces.append(GetKeyValue(GetSwitchKey(i)));

  EXPECT_EQ(expected_swithces, assembled_swithces);
}

TEST_F(CrashKeysTest, VariationsCapacity) {
  ASSERT_TRUE(InitVariationsCrashKeys());

  // Variation encoding: two 32bit numbers encorded as hex with a '-' separator.
  const char kSampleVariation[] = "12345678-12345678";
  const size_t kVariationLen = std::strlen(kSampleVariation);
  const size_t kSeparatedVariationLen = kVariationLen + 1U;
  ASSERT_EQ(17U, kVariationLen);

  // The expected capacity factors in a separator (',').
  const size_t kExpectedCapacity = 112U;
  ASSERT_EQ(kExpectedCapacity,
            crash_keys::kHugeSize / (kSeparatedVariationLen));

  // Create some variations and set the crash keys.
  std::vector<std::string> variations;
  for (size_t i = 0; i < kExpectedCapacity + 2; ++i)
    variations.push_back(kSampleVariation);
  crash_keys::SetVariationsList(variations);

  // Validate crash keys.
  ASSERT_TRUE(HasCrashKey(crash_keys::kNumVariations));
  EXPECT_EQ("114", GetKeyValue(crash_keys::kNumVariations));

  const size_t kExpectedChunks = (kSeparatedVariationLen * kExpectedCapacity) /
                                 crash_keys::kChunkMaxLength;
  for (size_t i = 0; i < kExpectedChunks; ++i) {
    ASSERT_TRUE(HasCrashKey(
        base::StringPrintf("%s-%" PRIuS, crash_keys::kVariations, i + 1)));
  }
}
