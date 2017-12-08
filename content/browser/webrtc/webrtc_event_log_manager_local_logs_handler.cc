// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager_local_logs_handler.h"
#include "content/public/browser/browser_thread.h"

namespace content {

namespace {
inline void MaybeReplyWithBool(const base::Location& from_here,
                               base::OnceCallback<void(bool)> reply,
                               bool value) {
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, from_here,
                            base::BindOnce(std::move(reply), value));
  }
}

struct MaybeReplyWithBoolOnExit final {
  MaybeReplyWithBoolOnExit(base::OnceCallback<void(bool)> reply,
                           bool default_value)
      : reply(std::move(reply)), value(default_value) {}
  ~MaybeReplyWithBoolOnExit() {
    if (reply) {
      MaybeReplyWithBool(FROM_HERE, std::move(reply), value);
    }
  }
  base::OnceCallback<void(bool)> reply;
  bool value;
};
}  // namespace

WebRtcEventLogManagerLocalLogsHandler::WebRtcEventLogManagerLocalLogsHandler(
    scoped_refptr<base::SequencedTaskRunner> task_runner) : task_runner_(task_runner) {
  
}

WebRtcEventLogManagerLocalLogsHandler::~WebRtcEventLogManagerLocalLogsHandler() {

}

}  // namespace content
