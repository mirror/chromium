// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_WATCH_TIME_SUB_REPORTER_H_
#define MEDIA_BLINK_WATCH_TIME_SUB_REPORTER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "media/base/timestamp_constants.h"

namespace media {

class WatchTimeReporter;
struct MediaLogEvent;

class WatchTimeSubReporter {
 public:
  WatchTimeSubReporter(WatchTimeReporter* watch_time_reporter);
  virtual ~WatchTimeSubReporter();

  virtual void OnStartReportingTimer(base::TimeDelta start_timestamp);
  void OnUpdateWatchTime(base::TimeDelta current_timestamp, MediaLogEvent*);

 protected:
  virtual void UpdateValueAfterCommit() = 0;
  void TriggerUpdate();

  // Helpers that are used by sub-classes. These are exposed in order to avoid
  // to make the WatchTimeReporter friend with all the sub-classes..
  bool IsBackground() const;
  bool HasVideo() const;
  bool TimerIsRunning() const;

  // TODO: comments
  base::TimeDelta start_timestamp_;
  base::TimeDelta end_timestamp_ = kNoTimestamp;
  base::TimeDelta last_timestamp_ = kNoTimestamp;

 private:
  virtual const char* GetRecordingKey() const = 0;
  virtual const char* GetFinalizeKey() const = 0;

  // Returns whether a change is expected to be commited.
  bool IsChangePending() const;

  // |watch_time_reporter_| owns |this|.
  WatchTimeReporter* watch_time_reporter_;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeSubReporter);
};

}  // namespace media

#endif  // MEDIA_BLINK_WATCH_TIME_SUB_REPORTER_H_
