// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "components/previews/core/previews_experiments.h"
#include "url/gurl.h"

namespace previews {

// Information needed for a log message. These information will be used to
// display log messages on chrome://interventions-internals.
struct MessageLog {
  std::string event_type;
  std::string event_description;
  GURL url;
  base::Time time;

  MessageLog(const std::string& event_type,
             const std::string& event_description,
             const GURL& url,
             const base::Time& time);

  MessageLog(const MessageLog& other);
};

// Information of a preview navigation.
struct PreviewNavigation {
  GURL url;
  PreviewsType type;
  bool opt_out;
  base::Time time;

  PreviewNavigation(const GURL& url,
                    PreviewsType type,
                    bool opt_out,
                    const base::Time& time);

  PreviewNavigation(const PreviewNavigation& other);
};

class PreviewsLog {
 public:
  PreviewsLog();
  ~PreviewsLog();

  const std::vector<MessageLog>& GetLogMessages();

  void LogMessage(const MessageLog& message);

  void LogPreviewNavigation(const PreviewNavigation& navigation);

 private:
  MessageLog ConvertToLogMessage(const PreviewNavigation& navigation);

  std::vector<MessageLog> log_messages_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsLog);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H
