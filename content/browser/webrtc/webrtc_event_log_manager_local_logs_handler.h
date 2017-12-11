// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_

namespace content {

class WebRtcEventLogManagerLocalLogsHandler {
 public:
  explicit WebRtcEventLogManagerLocalLogsHandler();
  ~WebRtcEventLogManagerLocalLogsHandler();

  bool PeerConnectionAdded(int render_process_id, int lid);

  bool PeerConnectionRemoved(int render_process_id, int lid);

  bool EnableLocalLogging(base::FilePath base_path, size_t max_file_size_bytes);

  bool DisableLocalLogging();

  void SetLocalLogsObserver(WebRtcLocalEventLogsObserver* observer);

  // This function is public, but this entire class is a protected
  // implementation detail of WebRtcEventLogManager, which hides this
  // function from everybody except its own unit-tests.
  void InjectClockForTesting(base::Clock* clock);

 protected:
  // Local log file handling.
  void StartLocalLogFile(int render_process_id, int lid);
  void StopLocalLogFile(int render_process_id, int lid);
  void WriteToLocalLogFile(int render_process_id,
                           int lid,
                           const std::string& output,
                           base::OnceCallback<void(bool)> reply);
  LocalLogFilesMap::iterator CloseLocalLogFile(LocalLogFilesMap::iterator it);

  // Derives the name of a local log file. The format is:
  // [user_defined]_[date]_[time]_[pid]_[lid].log
  base::FilePath GetLocalFilePath(const base::FilePath& base_path,
                                  int render_process_id,
                                  int lid);

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRtcEventLogManagerLocalLogsHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_LOCAL_LOGS_HANDLER_H_
