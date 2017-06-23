// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_TIME_TRACKER_H_
#define EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_TIME_TRACKER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <queue>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"

namespace base {
class Time;
}

// This class keeps monitors how much delay extensions add to network requests
// by using the webRequest API.
class ExtensionWebRequestTimeTracker {
 public:
  ExtensionWebRequestTimeTracker();
  ~ExtensionWebRequestTimeTracker();

  // Records the time that a request was created.
  void LogRequestStartTime(int64_t request_id, const base::Time& start_time);

  // Records the time that a request either completed or encountered an error.
  void LogRequestEndTime(int64_t request_id, const base::Time& end_time);

  // Records an additional delay for the given request caused by all extensions
  // combined.
  void IncrementTotalBlockTime(int64_t request_id,
                               const base::TimeDelta& block_time);

  // Called when an extension has canceled the given request.
  void SetRequestCanceled(int64_t request_id);

  // Called when an extension has redirected the given request to another URL.
  void SetRequestRedirected(int64_t request_id);

 private:
  // Timing information for a single request.
  struct RequestTimeLog {
    base::Time request_start_time;
    base::TimeDelta block_duration;

    RequestTimeLog();
    RequestTimeLog(const RequestTimeLog& other);
    ~RequestTimeLog();
  };

  // Called after a request finishes, to analyze the delays.
  void Analyze(int64_t request_id, base::TimeDelta request_duration);

  // A map of recent request IDs to timing info for each request.
  std::map<int64_t, RequestTimeLog> request_time_logs_;

  // A list of recent request IDs that we know about. Used to limit the size of
  // the logs.
  std::queue<int64_t> request_ids_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionWebRequestTimeTracker);
};

#endif  // EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_TIME_TRACKER_H_
