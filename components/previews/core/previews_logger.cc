// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_logger.h"

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "components/previews/core/previews_logger_observer.h"

namespace previews {

namespace {

const char kPreviewDecisionMade[] = "Decision";
const char kPreviewNavigationEventType[] = "Navigation";
const size_t kMaximumMessageLogs = 10;

std::string GetPreviewNavigationDescription(PreviewsType type, bool opt_out) {
  return base::StringPrintf("%s preview navigation - user opt_out: %s",
                            GetStringNameForType(type).c_str(),
                            opt_out ? "True" : "False");
}

std::string GetDecisionMadeDescription(PreviewsEligibilityReason reason,
                                       PreviewsType type) {
  std::string reason_str;
  switch (reason) {
    case PreviewsEligibilityReason::BLACKLIST_UNAVAILABLE:
      reason_str = "Blacklist unavailable";
      break;
    case PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED:
      reason_str = "Blacklist data not loaded";
      break;
    case PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT:
      reason_str = "User recently opted out";
      break;
    case PreviewsEligibilityReason::USER_BLACKLISTED:
      reason_str = "User blacklisted";
      break;
    case PreviewsEligibilityReason::HOST_BLACKLISTED:
      reason_str = "Host blacklisted";
      break;
    case PreviewsEligibilityReason::NETWORK_QUALITY_UNAVAILABLE:
      reason_str = "Network quality unavailable";
      break;
    case PreviewsEligibilityReason::NETWORK_NOT_SLOW:
      reason_str = "Network not slow";
      break;
    case PreviewsEligibilityReason::RELOAD_DISALLOWED:
      reason_str = "Reload disallowed";
      break;
    case PreviewsEligibilityReason::HOST_BLACKLISTED_BY_SERVER:
      reason_str = "Host blacklisted by server";
      break;
    default:
      reason_str = "Allowed";
  }
  return base::StringPrintf("%s preview decision made - %s",
                            GetStringNameForType(type).c_str(),
                            reason_str.c_str());
}

}  // namespace

PreviewsLogger::MessageLog::MessageLog(const std::string& event_type,
                                       const std::string& event_description,
                                       const GURL& url,
                                       base::Time time)
    : event_type(event_type),
      event_description(event_description),
      url(url),
      time(time) {}

PreviewsLogger::MessageLog::MessageLog(const MessageLog& other)
    : event_type(other.event_type),
      event_description(other.event_description),
      url(other.url),
      time(other.time) {}

PreviewsLogger::PreviewsLogger() {}

PreviewsLogger::~PreviewsLogger() {}

void PreviewsLogger::AddAndNotifyObserver(PreviewsLoggerObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.AddObserver(observer);
  for (auto message : log_messages_) {
    observer->OnNewMessageLogAdded(message);
  }
}

void PreviewsLogger::RemoveObserver(PreviewsLoggerObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.RemoveObserver(observer);
}

std::vector<PreviewsLogger::MessageLog> PreviewsLogger::log_messages() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return std::vector<MessageLog>(log_messages_.begin(), log_messages_.end());
}

void PreviewsLogger::LogMessage(const std::string& event_type,
                                const std::string& event_description,
                                const GURL& url,
                                base::Time time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_GE(kMaximumMessageLogs, log_messages_.size());
  MessageLog message(event_type, event_description, url, time);

  // Notify observers about the new MessageLog.
  for (auto& observer : observer_list_) {
    observer.OnNewMessageLogAdded(message);
  }

  if (log_messages_.size() >= kMaximumMessageLogs) {
    log_messages_.pop_front();
  }
  log_messages_.push_back(message);
}

void PreviewsLogger::LogPreviewNavigation(const GURL& url,
                                          PreviewsType type,
                                          bool opt_out,
                                          base::Time time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LogMessage(kPreviewNavigationEventType,
             GetPreviewNavigationDescription(type, opt_out), url, time);
}

void PreviewsLogger::LogPreviewsDecisionMade(PreviewsEligibilityReason reason,
                                             const GURL& url,
                                             base::Time time,
                                             PreviewsType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LogMessage(kPreviewDecisionMade, GetDecisionMadeDescription(reason, type),
             url, time);
}

}  // namespace previews
