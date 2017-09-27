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

// Information needed for a log message. These information will be used to
// display log messages on chrome://interventions-internals.
struct LogMessage {
  std::string event_type;
  std::string event_description;
  GURL url;
  base::Time time;

  LogMessage(const std::string& event_type,
             const std::string& event_description,
             const GURL& url,
             const base::Time& time);

  LogMessage(const LogMessage& other);
};

// Information of a preview navigation.
struct PreviewNavigation {
  GURL url;
  PreviewsType type;
  bool opt_out;
  base::Time time;

  PreviewNavigation(const GURL& url,
                    const PreviewsType& type,
                    bool opt_out,
                    const base::Time& time);

  PreviewNavigation(const PreviewNavigation& other);
};

class PreviewsLog {
 public:
  PreviewsLog();
  ~PreviewsLog();

  const std::vector<LogMessage>& GetLogMessages();

  void AddMessage(const LogMessage& message);

  void AddPreviewNavigation(const PreviewNavigation& navigation);

 private:
  LogMessage ConvertToLogMessage(const PreviewNavigation& navigation);

  std::vector<LogMessage> log_messages_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsLog);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_H
