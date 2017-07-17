// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/entry.h"
#include "base/time/time.h"
#include "components/download/internal/test/download_params_utils.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {

// Tests Entry assignemnt, comparison, and constructor from another Entry.
TEST(DownloadServiceEntryTest, Operators) {
  Entry entry1(test::BuildBasicDownloadParams());
  Entry entry2 = entry1;

  for (int current_test = 0; true; current_test++) {
    switch (current_test) {
      case 0: {
        entry1.guid.pop_back();
        break;
      }
      case 1: {
        entry1.scheduling_params.cancel_time = base::Time::Now();
        entry2.scheduling_params.cancel_time =
            base::Time::Now() - base::TimeDelta::FromDays(1);
        break;
      }
      case 2: {
        entry1.scheduling_params.network_requirements =
            SchedulingParams::NetworkRequirements::OPTIMISTIC;
        entry2.scheduling_params.network_requirements =
            SchedulingParams::NetworkRequirements::UNMETERED;
        break;
      }
      case 3: {
        entry1.scheduling_params.battery_requirements =
            SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE;
        entry2.scheduling_params.battery_requirements =
            SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
        break;
      }
      case 4: {
        entry1.scheduling_params.priority = SchedulingParams::Priority::LOW;
        entry2.scheduling_params.priority = SchedulingParams::Priority::HIGH;
        break;
      }
      case 5: {
        entry1.request_params.url = GURL("http://www.test1.com");
        entry2.request_params.url = GURL("http://www.test2.com");
        break;
      }
      case 6: {
        entry1.request_params.method = "A";
        entry2.request_params.method = "B";
        break;
      }
      case 7: {
        entry1.request_params.request_headers.SetHeader("test_header_key_A",
                                                        "test_header_value_A");
        entry2.request_params.request_headers.SetHeader("test_header_key_B",
                                                        "test_header_value_B");
        break;
      }
      case 8: {
        entry1.state = Entry::State::NEW;
        entry2.state = Entry::State::ACTIVE;
        break;
      }
      case 9: {
        entry1.create_time = base::Time::Now();
        entry2.create_time = base::Time::Now() - base::TimeDelta::FromDays(1);
        break;
      }
      case 10: {
        entry1.completion_time = base::Time::Now();
        entry2.completion_time =
            base::Time::Now() - base::TimeDelta::FromDays(1);
        break;
      }
      case 11: {
        entry1.attempt_count++;
        break;
      }
      case 12: {
        entry1.set_traffic_annotation(TRAFFIC_ANNOTATION_FOR_TESTS);
        entry2.reset_traffic_annotation();
      }
      default:
        current_test = -1;
    }

    if (current_test < 0)
      break;

    EXPECT_FALSE(entry1 == entry2);
    entry2 = entry1;
    EXPECT_TRUE(entry1 == entry2);
    EXPECT_TRUE(entry1 == Entry(entry1));
  }
}

}  // namespace download
