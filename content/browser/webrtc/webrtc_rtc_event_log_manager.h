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
#include "content/common/content_export.h"

namespace content {

// This is a singleton class running in the browser UI thread.
// It is in charge of writing RTC event logs to temporary files, then uploading
// those files to remote servers, as well as of writing the logs to files which
// were manually indicated by the user from the WebRTCIntenals. (A log may
// simulatenously be written to both, either, or none.)
// TODO(eladalon): Currently, only write to files. Add the code that also
// uploads those files.
class CONTENT_EXPORT RtcEventLogManager {
 public:
  static RtcEventLogManager* GetInstance();

  RtcEventLogManager();
  ~RtcEventLogManager();

  // TODO(eladalon): In this CL, we only support the old scheme - manual
  // logs initiated by the user through WebRTCInternals, which are only stored
  // locally. An upcoming CL will allow us to also start/stop an RTC event log
  // that will be uploaded to the server.
  void LocalRtcEventLogStart(int render_process_id, int lid,
                             const base::FilePath& base_path);
  void LocalRtcEventLogStop(int render_process_id, int lid);

  // Called when a new log fragment is sent from the renderer. This will
  // potentially be written to a local RTC event log, a log destined for upload,
  // or both.
  void OnRtcEventLogWrite(int render_process_id,
                          int lid,
                          const std::string& output);

 private:
  friend struct base::LazyInstanceTraitsBase<RtcEventLogManager>;

  // Handling of local log files (local-user-initiated; as opposed to logs
  // requested by JS and bound to be uploaded to a remote server).
  void StartLocalLogFile(int render_process_id, int lid,
                         const base::FilePath& base_path);
  void StopLocalLogFile(int render_process_id, int lid);
  void WriteToLocalLogFile(int render_process_id,
                           int lid,
                           const std::string& output);

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
    int lid;
  };

  // TODO(eladalon): An upcoming CL will add an additional container with
  // logs that will be uploaded to the server.
  std::map<PeerConnectionKey, base::File> pc_to_local_log_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::WeakPtrFactory<RtcEventLogManager> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
