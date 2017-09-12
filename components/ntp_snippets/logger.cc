// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/logger.h"

#include <vector>

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "components/ntp_snippets/features.h"

namespace ntp_snippets {

namespace {

const int MAX_ITEMS_COUNT = 1000;

}  // namespace

Logger::Logger() = default;
Logger::~Logger() = default;

void Logger::Log(const std::string& message) {
  if (!base::FeatureList::IsEnabled(kContentSuggestionsDebugLog)) {
    return;
  }

  LogItem item;
  item.message = message;
  item.time = base::Time::Now();
  logged_items_.push_back(item);
  if (logged_items_.size() > MAX_ITEMS_COUNT) {
    logged_items_.pop_front();
  }
}

std::string Logger::GetHumanReadableLog() const {
  std::vector<std::string> message_strings;
  for (const LogItem& item : logged_items_) {
    message_strings.push_back(item.ToString());
  }
  return base::JoinString(message_strings, /*separator=*/"\n");
}

std::string Logger::LogItem::ToString() const {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  std::string time_string = base::StringPrintf(
      "%04d-%02d-%02d %02d:%02d:%02d.%03d UTC", exploded.year, exploded.month,
      exploded.day_of_month, exploded.hour, exploded.minute, exploded.second,
      exploded.millisecond);
  return base::JoinString({time_string, message}, /*separator=*/" ");
}

}  // namespace ntp_snippets
