// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_

#include <map>
#include <set>
#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/time/clock.h"
#include "content/browser/webrtc/webrtc_event_log_manager_common.h"

// TODO: !!! Order

namespace content {

class WebRtcEventLogManagerLocalLogsHandler {
 public:
  explicit WebRtcEventLogManagerLocalLogsHandler();
  ~WebRtcEventLogManagerLocalLogsHandler();

  bool PeerConnectionAdded(int render_process_id, int lid);

  bool PeerConnectionRemoved(int render_process_id, int lid);

  bool EnableLocalLogging(base::FilePath base_path, size_t max_file_size_bytes);

  bool DisableLocalLogging();

  bool WriteToLocalLogFile(int render_process_id,
                           int lid,
                           const std::string& output);

  void SetLocalLogsObserver(WebRtcLocalEventLogsObserver* observer);

  // This function is public, but this entire class is a protected
  // implementation detail of WebRtcEventLogManager, which hides this
  // function from everybody except its own unit-tests.
  void InjectClockForTesting(base::Clock* clock);

 protected:
  struct LogFile {
    LogFile(base::File file, size_t max_file_size_bytes)
        : file(std::move(file)),
          max_file_size_bytes(max_file_size_bytes),
          file_size_bytes(0) {}
    base::File file;
    const size_t max_file_size_bytes;
    size_t file_size_bytes;
  };

  typedef std::map<PeerConnectionKey, LogFile> LocalLogFilesMap;

  // Local log file handling.
  void StartLocalLogFile(int render_process_id, int lid);
  void StopLocalLogFile(int render_process_id, int lid);
  LocalLogFilesMap::iterator CloseLocalLogFile(LocalLogFilesMap::iterator it);

  // Derives the name of a local log file. The format is:
  // [user_defined]_[date]_[time]_[pid]_[lid].log
  base::FilePath GetFilePath(const base::FilePath& base_path,
                             int render_process_id,
                             int lid);

  // For unit tests only, and specifically for unit tests that verify the
  // filename format (derived from the current time as well as the renderer PID
  // and PeerConnection local ID), we want to make sure that the time and date
  // cannot change between the time the clock is read by the unit under test
  // (namely WebRtcEventLogManager) and the time it's read by the test.
  base::Clock* clock_for_testing_;

  // Currently active peer connections. PeerConnections which have been closed
  // are not considered active, regardless of whether they have been torn down.
  std::set<PeerConnectionKey> active_peer_connections_;

  // Local log files, stored at the behest of the user (via WebRTCInternals).
  // TODO(eladalon): Add an additional container with logs that will be uploaded
  // to the server. https://crbug.com/775415
  LocalLogFilesMap local_logs_;

  // Observer which will be informed whenever a local log file is started or
  // stopped. Its callbacks are called synchronously from |file_task_runner_|,
  // so the observer needs to be able to either run from any (sequenced) runner.
  WebRtcLocalEventLogsObserver* observer_;

  // If |base_path_| is empty, local logging is disabled.
  // If nonempty, local logging is enabled, and all local logs will be saved
  // to this directory.
  base::FilePath base_path_;

  // The maximum size for local logs, in bytes. Note that
  // kWebRtcEventLogManagerUnlimitedFileSize is a sentinel value (with a
  // self-explanatory name).
  size_t max_log_file_size_bytes_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRtcEventLogManagerLocalLogsHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_
