// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/tether_session_completion_logger.h"

#include <memory>

#include "base/test/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

class TetherSessionCompletionLoggerTest : public testing::Test {
 protected:
  TetherSessionCompletionLoggerTest() = default;
  ~TetherSessionCompletionLoggerTest() override = default;

  void SetUp() override {
    logger_ = std::make_unique<TetherSessionCompletionLogger>();
  }

  void TestSessionCompletionReasonRecorded(
      TetherSessionCompletionLogger::SessionCompletionReason
          expected_session_completion_reason) {
    logger_->RecordTetherSessionCompletion(expected_session_completion_reason);
    histogram_tester_.ExpectBucketCount(
        "InstantTethering.SessionCompletionReason",
        expected_session_completion_reason, 1);
  }

  std::unique_ptr<TetherSessionCompletionLogger> logger_;

  base::HistogramTester histogram_tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TetherSessionCompletionLoggerTest);
};

TEST_F(TetherSessionCompletionLoggerTest, TestOther) {
  TestSessionCompletionReasonRecorded(
      TetherSessionCompletionLogger::SessionCompletionReason::OTHER);
}

}  // namespace tether

}  // namespace chromeos
