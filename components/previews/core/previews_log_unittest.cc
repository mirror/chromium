// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_log.h"

#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/test/simple_test_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kPreviewsNavigationEventType[] = "Navigation";
const size_t kMaximumMessageLogs = 10;

}  // namespace

namespace previews {

class PreviewsLogTest : public testing::Test {
 public:
  PreviewsLogTest() {}

  ~PreviewsLogTest() override {}

  void SetUp() override {
    test_clock_ = base::MakeUnique<base::SimpleTestClock>();
    logger = base::MakeUnique<PreviewsLog>();
  }

 protected:
  std::unique_ptr<PreviewsLog> logger;
  std::unique_ptr<base::SimpleTestClock> test_clock_;
};

TEST_F(PreviewsLogTest, AddLogMessageToLogger) {
  size_t expected_size = 0;
  EXPECT_EQ(expected_size, logger->log_messages().size());

  const std::string event_type_a = "Event_a";
  const std::string event_type_b = "Event_b";
  const std::string event_type_c = "Event_c";
  const std::string event_description_a = "Some description a";
  const std::string event_description_b = "Some description b";
  const std::string event_description_c = "Some description c";
  const base::Time time = test_clock_->Now();
  const GURL url_a("http://www.url_a.com/url_a");
  const GURL url_b("http://www.url_b.com/url_b");
  const GURL url_c("http://www.url_c.com/url_c");

  logger->LogMessage(event_type_a, event_description_a, url_a, time);
  logger->LogMessage(event_type_b, event_description_b, url_b, time);
  logger->LogMessage(event_type_c, event_description_c, url_c, time);

  expected_size = 3;
  std::vector<MessageLog> actual = logger->log_messages();

  EXPECT_EQ(expected_size, actual.size());

  EXPECT_EQ(event_type_a, actual[0].event_type);
  EXPECT_EQ(event_description_a, actual[0].event_description);
  EXPECT_EQ(time, actual[0].time);
  EXPECT_EQ(url_a, actual[0].url);

  EXPECT_EQ(event_type_b, actual[1].event_type);
  EXPECT_EQ(event_description_b, actual[1].event_description);
  EXPECT_EQ(time, actual[1].time);
  EXPECT_EQ(url_b, actual[1].url);

  EXPECT_EQ(event_type_c, actual[2].event_type);
  EXPECT_EQ(event_description_c, actual[2].event_description);
  EXPECT_EQ(time, actual[2].time);
  EXPECT_EQ(url_c, actual[2].url);
}

TEST_F(PreviewsLogTest, LogPreviewNavigationLogMessage) {
  const base::Time time = test_clock_->Now();

  PreviewsType type_a = PreviewsType::OFFLINE;
  PreviewsType type_b = PreviewsType::LOFI;
  const GURL url_a("http://www.url_a.com/url_a");
  const GURL url_b("http://www.url_b.com/url_b");

  PreviewNavigation navigation_a(url_a, type_a, true, time);
  PreviewNavigation navigation_b(url_b, type_b, false, time);

  logger->LogPreviewNavigation(navigation_a);
  logger->LogPreviewNavigation(navigation_b);

  std::vector<MessageLog> actual = logger->log_messages();

  size_t expected_size = 2;
  EXPECT_EQ(expected_size, actual.size());

  std::string expected_description_a =
      "Offline preview navigation - user opt_out: True";
  EXPECT_EQ(kPreviewsNavigationEventType, actual[0].event_type);
  EXPECT_EQ(expected_description_a, actual[0].event_description);
  EXPECT_EQ(url_a, actual[0].url);
  EXPECT_EQ(time, actual[0].time);

  std::string expected_description_b =
      "LoFi preview navigation - user opt_out: False";
  EXPECT_EQ(kPreviewsNavigationEventType, actual[1].event_type);
  EXPECT_EQ(expected_description_b, actual[1].event_description);
  EXPECT_EQ(url_b, actual[1].url);
  EXPECT_EQ(time, actual[1].time);
}

TEST_F(PreviewsLogTest, PreviewsLogOnlyKeepsCertainNumberOfMessageLogs) {
  const std::string event_type = "Event";
  const std::string event_description = "Some description";
  const base::Time time = test_clock_->Now();
  const GURL url("http://www.url_.com/url_");

  for (size_t i = 0; i < 2 * kMaximumMessageLogs; i++) {
    logger->LogMessage(event_type, event_description, url, time);
  }

  EXPECT_EQ(kMaximumMessageLogs, logger->log_messages().size());
}

}  // namespace previews
