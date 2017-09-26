// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "components/previews/core/previews_experiments.h"
#include "url/gurl.h"

namespace previews {

// The description of a previews navigation.
struct PreviewNavigation {
  GURL url;
  PreviewsType type;
  bool opt_out;
  base::Time time;
};

// Information needed for a log message. These information will be used to
// display log messages on chrome://interventions-internals.
struct LogMessage {
  std::string event_type;
  std::string event_description;
  GURL url;
  base::Time time;
};

class PreviewsLog {
 public:
  PreviewsLog(std::unique_ptr<base::Clock> clock);

  ~PreviewsLog();

  // Add |navigation| to the list of log messages in PreviewsLog.
  void LogPreviewsNavigation(PreviewNavigation navigation);

  // Retrieve a vector of LogMessage, with the |number_of_message| most recent
  // LogMessages.
  unique_ptr<std::vector<LogMessage>> GetLogMessages(int number_of_messages);

 private:
  // Convert a PreviewsNavigation to the general LogMessage.
  LogMessage GetLogMessage(PreviewNavigation navigation);

  std::unique_ptr<base::Clock> clock_;

  // Storing all log messages posted to the PreviewsLog.
  std::vector<LogMessage> log_messages_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsLog);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H
