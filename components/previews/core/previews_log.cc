// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_log.h"

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"

namespace {

const char kPreviewNavigationEventType[] = "Navigation";
const size_t kMaximumMessageLogs = 10;

}  // namespace

namespace previews {

MessageLog::MessageLog(const std::string& event_type,
                       const std::string& event_description,
                       const GURL& url,
                       base::Time time)
    : event_type(event_type),
      event_description(event_description),
      url(url),
      time(time) {}

MessageLog::MessageLog(const MessageLog& other)
    : event_type(other.event_type),
      event_description(other.event_description),
      url(other.url),
      time(other.time) {}

PreviewsLogger::PreviewsLogger() {}

PreviewsLogger::~PreviewsLogger() {}

std::vector<MessageLog> PreviewsLogger::log_messages() const {
  return std::vector<MessageLog>(log_messages_.begin(), log_messages_.end());
}

void PreviewsLogger::LogMessage(const std::string& event_type,
                                const std::string& event_description,
                                const GURL& url,
                                base::Time time) {
  DCHECK_GE(kMaximumMessageLogs, log_messages_.size());
  if (log_messages_.size() >= kMaximumMessageLogs) {
    log_messages_.pop_front();
  }
  log_messages_.emplace_back(event_type, event_description, url, time);
}

void PreviewsLogger::LogPreviewNavigation(const PreviewNavigation& navigation) {
  std::string event_description = GetPreviewNavigationDescription(navigation);
  LogMessage(kPreviewNavigationEventType, event_description, navigation.url,
             navigation.time);
}

std::string PreviewsLogger::GetPreviewNavigationDescription(
    const PreviewNavigation& navigation) const {
  return base::StringPrintf("%s preview navigation - user opt_out: %s",
                            GetStringNameForType(navigation.type).c_str(),
                            navigation.opt_out ? "True" : "False");
}

}  // namespace previews
