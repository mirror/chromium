// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_

#include <map>
#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/clock.h"
#include "content/common/content_export.h"

namespace content {

// This is a singleton class running in the browser UI thread.
// It is in charge of writing RTC event logs to temporary files, then uploading
// those files to remote servers, as well as of writing the logs to files which
// were manually indicated by the user from the WebRTCIntenals. (A log may
// simulatenously be written to both, either, or none.)
// TODO(eladalon): This currently only supports the old use-case - locally
// stored log files. An upcoming CL will add remote-support.
class CONTENT_EXPORT RtcEventLogManager {
 public:
  static constexpr size_t kUnlimitedFileSize = 0;

  static RtcEventLogManager* GetInstance();

  RtcEventLogManager();
  ~RtcEventLogManager();

  // TODO(eladalon): In this CL, we only support the old scheme - manual
  // logs initiated by the user through WebRTCInternals, which are only stored
  // locally. An upcoming CL will allow us to also start/stop an RTC event log
  // that will be uploaded to the server.
  void LocalRtcEventLogStart(int render_process_id,
                             int lid,  // Renderer-local PeerConnection ID.
                             const base::FilePath& base_path,
                             size_t max_file_size);
  void LocalRtcEventLogStop(int render_process_id, int lid);

  // Called when a new log fragment is sent from the renderer. This will
  // potentially be written to a local RTC event log, a log destined for upload,
  // or both.
  void OnRtcEventLogWrite(int render_process_id,
                          int lid,  // Renderer-local PeerConnection ID.
                          const std::string& output);

 private:
  friend struct base::LazyInstanceTraitsBase<RtcEventLogManager>;
  friend class RtcEventLogManagerTest;

  void InjectClockForTesting(base::Clock* clock);

  base::FilePath GetLocalFilePath(const base::FilePath& base_path,
                                  int render_process_id,
                                  int lid);

  // For a given Chrome session, this is a unique key for PeerConnections.
  // It's not, however, unique between sessions (after Chrome is restarted).
  struct PeerConnectionKey {
    bool operator<(const PeerConnectionKey& other) const {
      if (render_process_id != other.render_process_id) {
        return render_process_id < other.render_process_id;
      }
      return lid < other.lid;
    }
    int render_process_id;
    int lid;  // Renderer-local PeerConnection ID.
  };

  struct LogFile {
    LogFile(base::File file, size_t max_file_size)
        : file(std::move(file)), max_file_size(max_file_size), file_size(0) {}
    base::File file;
    const size_t max_file_size;
    size_t file_size;
  };

  typedef std::map<PeerConnectionKey, LogFile> LocalLogFilesMap;

  // Handling of local log files (local-user-initiated; as opposed to logs
  // requested by JS and bound to be uploaded to a remote server).
  void StartLocalLogFile(int render_process_id,
                         int lid,  // Renderer-local PeerConnection ID.
                         const base::FilePath& base_path,
                         size_t max_file_size);
  void StopLocalLogFile(int render_process_id, int lid);
  void CloseLocalLogFile(LocalLogFilesMap::iterator it);
  void WriteToLocalLogFile(int render_process_id,
                           int lid,
                           const std::string& output);

  // For unit-tests only, we want to both know what the time ended up being,
  // as well as be able to control it for this module only, without affecting
  // the behavior of other modules (as we would have had to do if we were to
  // modify the global clock to a fake clock).
  base::Clock* clock_for_testing_;

  // Local log files, stored at the behest of the user (via WebRTCInternals).
  // TODO(eladalon): An upcoming CL will add an additional container with
  // logs that will be uploaded to the server.
  LocalLogFilesMap local_logs_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::WeakPtrFactory<RtcEventLogManager> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
