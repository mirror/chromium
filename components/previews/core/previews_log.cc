// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_log.h"

#include "base/memory/ptr_util.h"

namespace {

const char kPreviewNavigationEventType[] = "NAVIGATION";

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

PreviewsLog::PreviewsLog() {}

PreviewsLog::~PreviewsLog() {}

const std::vector<MessageLog>& PreviewsLog::GetLogMessages() {
  return log_messages_;
}

void PreviewsLog::LogMessage(const MessageLog& message) {
  log_messages_.push_back(message);
}

void PreviewsLog::LogPreviewNavigation(const PreviewNavigation& navigation) {
  LogMessage(ConvertToLogMessage(navigation));
}

MessageLog PreviewsLog::ConvertToLogMessage(
    const PreviewNavigation& navigation) {
  std::string event_type = kPreviewNavigationEventType;
  std::string event_description = navigation.url.spec() + " - opted out = " +
                                  (navigation.opt_out ? "True" : "False");

  MessageLog message(event_type, event_description, navigation.url,
                     navigation.time);
  return message;
}

}  // namespace previews
