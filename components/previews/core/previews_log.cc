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
                       const base::Time& time)
    : event_type(event_type),
      event_description(event_description),
      url(url),
      time(time) {}

MessageLog::MessageLog(const MessageLog& other)
    : event_type(other.event_type),
      event_description(other.event_description),
      url(GURL(other.url)),
      time(other.time) {}

PreviewNavigation::PreviewNavigation(const GURL& url,
                                     PreviewsType type,
                                     bool opt_out,
                                     const base::Time& time)
    : url(url), type(type), opt_out(opt_out), time(time) {}

PreviewNavigation::PreviewNavigation(const PreviewNavigation& other)
    : url(GURL(other.url)),
      type(other.type),
      opt_out(other.opt_out),
      time(other.time) {}

std::string PreviewNavigation::ToString() const {
  return base::StringPrintf("%s preview navigation - user opt_out: %s",
                            GetStringNameForType(type).c_str(),
                            opt_out ? "True" : "False");
}

PreviewsLog::PreviewsLog() {}

PreviewsLog::~PreviewsLog() {}

const std::vector<MessageLog>& PreviewsLog::log_messages() {
  return log_messages_;
}

void PreviewsLog::LogMessage(const std::string& event_type,
                             const std::string& event_description,
                             const GURL& url,
                             const base::Time& time) {
  if (log_messages_.size() >= kMaximumMessageLogs) {
    log_messages_.erase(log_messages_.begin());
  }
  log_messages_.push_back(MessageLog(event_type, event_description, url, time));
}

void PreviewsLog::LogPreviewNavigation(const PreviewNavigation& navigation) {
  std::string event_type = kPreviewNavigationEventType;
  std::string event_description = navigation.ToString();
  LogMessage(event_type, event_description, navigation.url, navigation.time);
}

}  // namespace previews
