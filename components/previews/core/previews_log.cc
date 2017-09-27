// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_log.h"

#include "base/memory/ptr_util.h"

namespace {

const char kPreviewNavigationEventType[] = "NAVIGATION";

}  // namespace

namespace previews {

LogMessage::LogMessage(const std::string& event_type,
                       const std::string& event_description,
                       const GURL& url,
                       const base::Time& time)
    : event_type(event_type),
      event_description(event_description),
      url(url),
      time(time) {}

LogMessage::LogMessage(const LogMessage& other)
    : event_type(other.event_type),
      event_description(other.event_description),
      url(GURL(other.url)),
      time(other.time) {}

PreviewNavigation::PreviewNavigation(const GURL& url,
                                     const PreviewsType& type,
                                     bool opt_out,
                                     const base::Time& time)
    : url(url), type(type), opt_out(opt_out), time(time) {}

PreviewsLog::PreviewsLog() {}

PreviewsLog::~PreviewsLog() {}

const std::vector<LogMessage>& PreviewsLog::GetLogMessages() {
  return log_messages_;
}

void PreviewsLog::AddMessage(const LogMessage& message) {
  log_messages_.push_back(message);
}

void PreviewsLog::AddPreviewNavigation(const PreviewNavigation& navigation) {
  AddMessage(ConvertToLogMessage(navigation));
}

LogMessage PreviewsLog::ConvertToLogMessage(
    const PreviewNavigation& navigation) {
  std::string event_type = kPreviewNavigationEventType;
  std::string event_description = navigation.url.spec() + " - opted out = " +
                                  (navigation.opt_out ? "True" : "False");

  LogMessage message(event_type, event_description, navigation.url,
                     navigation.time);
  return message;
}

}  // namespace previews
