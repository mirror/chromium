// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/entry.h"
#include "base/time/time.h"
#include "components/download/internal/test/download_params_utils.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {

void VerifyOperators(const Entry& entry1, Entry* entry2) {
  // Compare unequal values.
  EXPECT_FALSE(entry1 == *entry2);
  EXPECT_FALSE(*entry2 == entry1);
  EXPECT_NE(entry1, *entry2);
  EXPECT_NE(*entry2, entry1);

  // Verify assignment.
  *entry2 = entry1;
  EXPECT_TRUE(entry1 == *entry2);
  EXPECT_TRUE(*entry2 == entry1);
  EXPECT_EQ(entry1, *entry2);
  EXPECT_EQ(*entry2, entry1);

  // Verify copy constructor.
  Entry entry3(entry1);
  EXPECT_EQ(entry1, entry3);
  EXPECT_EQ(entry3, entry1);
}

// Tests Entry assignment operator, comparison operator, and constructor from
// another Entry.
TEST(DownloadServiceEntryTest, Operators) {
  Entry entry1(test::BuildBasicDownloadParams());
  Entry entry2 = entry1;

  entry1.guid.pop_back();
  VerifyOperators(entry1, &entry2);

  entry1.scheduling_params.cancel_time = base::Time::Now();
  entry2.scheduling_params.cancel_time =
      base::Time::Now() - base::TimeDelta::FromDays(1);
  VerifyOperators(entry1, &entry2);

  entry1.scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::OPTIMISTIC;
  entry2.scheduling_params.network_requirements =
      SchedulingParams::NetworkRequirements::UNMETERED;
  VerifyOperators(entry1, &entry2);

  entry1.scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
  entry2.scheduling_params.battery_requirements =
      SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
  VerifyOperators(entry1, &entry2);

  entry1.scheduling_params.priority = SchedulingParams::Priority::LOW;
  entry2.scheduling_params.priority = SchedulingParams::Priority::HIGH;
  VerifyOperators(entry1, &entry2);

  entry1.request_params.url = GURL("http://www.test1.com");
  entry2.request_params.url = GURL("http://www.test2.com");
  VerifyOperators(entry1, &entry2);

  entry1.request_params.method = "A";
  entry2.request_params.method = "B";
  VerifyOperators(entry1, &entry2);

  entry1.request_params.request_headers.SetHeader("test_header_key_A",
                                                  "test_header_value_A");
  entry2.request_params.request_headers.SetHeader("test_header_key_B",
                                                  "test_header_value_B");
  VerifyOperators(entry1, &entry2);

  entry1.state = Entry::State::NEW;
  entry2.state = Entry::State::ACTIVE;
  VerifyOperators(entry1, &entry2);

  entry1.create_time = base::Time::Now();
  entry2.create_time = base::Time::Now() - base::TimeDelta::FromDays(1);
  VerifyOperators(entry1, &entry2);

  entry1.completion_time = base::Time::Now();
  entry2.completion_time = base::Time::Now() - base::TimeDelta::FromDays(1);
  VerifyOperators(entry1, &entry2);

  entry1.attempt_count++;
  VerifyOperators(entry1, &entry2);
}

}  // namespace download
