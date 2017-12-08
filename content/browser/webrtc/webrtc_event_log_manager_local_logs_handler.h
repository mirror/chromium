// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_

#include "base/memory/scoped_refptr.h"
#include "base/sequenced_task_runner.h"

namespace content {

class WebRtcEventLogManagerLocalLogsHandler {
 public:
  explicit WebRtcEventLogManagerLocalLogsHandler(scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~WebRtcEventLogManagerLocalLogsHandler();


 protected:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRtcEventLogManagerLocalLogsHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_
