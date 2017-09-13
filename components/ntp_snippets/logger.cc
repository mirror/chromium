// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/logger.h"

#include <vector>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "components/ntp_snippets/features.h"

namespace ntp_snippets {

namespace {

// Variation parameter for maximal number of the most recent log items for the
// logger to store.
const char kMaxItemsCountParamName[] = "max_items_count";
const int kMaxItemsCountDefault = 1000;

int GetMaxItemsCount() {
  return base::GetFieldTrialParamByFeatureAsInt(kContentSuggestionsDebugLog,
                                                kMaxItemsCountParamName,
                                                kMaxItemsCountDefault);
}

}  // namespace

Logger::Logger() = default;
Logger::~Logger() = default;

void Logger::Log(const base::Location& from_here, const std::string& message) {
  if (!base::FeatureList::IsEnabled(kContentSuggestionsDebugLog)) {
    return;
  }

  LogItem item;
  item.message = message;
  item.time = base::Time::Now();
  item.from_where = from_here;
  logged_items_.push_back(item);
  if (static_cast<int>(logged_items_.size()) > GetMaxItemsCount()) {
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
  return base::StringPrintf("%s %s %s", time_string.c_str(),
                            from_where.ToString().c_str(), message.c_str());
}

}  // namespace ntp_snippets
