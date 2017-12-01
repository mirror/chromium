// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_

#include <map>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/clock.h"
#include "content/common/content_export.h"

class WebrtcEventLogApiTest;

namespace content {

// This is a singleton class running in the browser UI thread.
// It is in charge of writing RTC event logs to temporary files, then uploading
// those files to remote servers, as well as of writing the logs to files which
// were manually indicated by the user from the WebRTCIntenals. (A log may
// simulatenously be written to both, either, or none.)
// TODO(eladalon): This currently only supports the old use-case - locally
// stored log files. An upcoming CL will add remote-support.
// https://crbug.com/775415
class CONTENT_EXPORT WebRtcEventLogManager {
 public:
  // For a given Chrome session, this is a unique key for PeerConnections.
  // It's not, however, unique between sessions (after Chrome is restarted).
  struct PeerConnectionKey {
    PeerConnectionKey(int render_process_id, int lid)
        : render_process_id(render_process_id), lid(lid) {}
    bool operator<(const PeerConnectionKey& other) const {
      if (render_process_id != other.render_process_id) {
        return render_process_id < other.render_process_id;
      }
      return lid < other.lid;
    }
    int render_process_id;
    int lid;  // Renderer-local PeerConnection ID.
  };

  // Allow observers to register for notifications of local log files being
  // started/stopped, and the paths which will be used for these logs.
  class LocalLogsObserver {
   public:
    virtual ~LocalLogsObserver() = 0;
    virtual void OnLocalLogsStarted(
        std::map<PeerConnectionKey, base::FilePath>& local_logs) = 0;
    virtual void OnLocalLogsStopped(
        std::set<PeerConnectionKey>& local_logs) = 0;
  };

  static constexpr size_t kUnlimitedFileSize = 0;

  static WebRtcEventLogManager* GetInstance();

  // TODO: !!!
  // Currently, we only support manual logs initiated by the user
  // through WebRTCInternals, which are stored locally.
  // TODO(eladalon): Allow starting/stopping an RTC event log
  // that will be uploaded to the server. https://crbug.com/775415

  // TODO: !!!
  void PeerConnectionAdded(
      int render_process_id,
      int lid,  // Renderer-local PeerConnection ID.
      base::OnceCallback<void(bool)> reply = base::OnceCallback<void(bool)>());

  // TODO: !!!
  void PeerConnectionRemoved(
      int render_process_id,
      int lid,  // Renderer-local PeerConnection ID.
      base::OnceCallback<void(bool)> reply = base::OnceCallback<void(bool)>());

  // TODO: !!!
  // TODO: !!! Mention also how there's no guarantee about the order; we just
  // set as many as we can, in some random order.
  void EnableLocalLogging(
      base::FilePath base_path,
      size_t max_file_size_bytes,
      base::OnceCallback<void(bool)> reply = base::OnceCallback<void(bool)>());

  // TODO: !!!
  void DisableLocalLogging(
      base::OnceCallback<void(bool)> reply = base::OnceCallback<void(bool)>());

  // TODO: !!!
  // Called when a new log fragment is sent from the renderer. This will
  // potentially be written to a local WebRTC event log, a log destined for
  // upload, or both.
  void OnWebRtcEventLogWrite(
      int render_process_id,
      int lid,  // Renderer-local PeerConnection ID.
      const std::string& output,
      base::OnceCallback<void(bool)> reply = base::OnceCallback<void(bool)>());

  // TODO: !!!
  void SetLocalLogsObserver(
      LocalLogsObserver* observer,
      base::OnceCallback<void()> reply = base::OnceCallback<void()>());

 protected:
  friend class ::WebrtcEventLogApiTest;
  friend class WebRtcEventLogManagerTest;  // unit tests inject a frozen clock.
  // TODO(eladalon): Remove WebRtcEventLogHostTest and this friend declaration.
  // https://crbug.com/775415
  friend class WebRtcEventlogHostTest;
  friend struct base::LazyInstanceTraitsBase<WebRtcEventLogManager>;

  WebRtcEventLogManager();
  ~WebRtcEventLogManager();

  // Only used for testing, so we have no threading concenrs here; it should
  // always be called before anything that might post to the internal TQ.
  void InjectClockForTesting(base::Clock* clock);

  base::FilePath GetLocalFilePath(const base::FilePath& base_path,
                                  int render_process_id,
                                  int lid);

  struct LogFile {
    LogFile(base::File file, size_t max_file_size_bytes)
        : file(std::move(file)),
          max_file_size_bytes(max_file_size_bytes),
          file_size_bytes(0) {}
    base::File file;
    const size_t max_file_size_bytes;
    size_t file_size_bytes;
  };

  // TODO: !!! Order
  // TODO: !!! Names (*Internal)

  typedef std::map<PeerConnectionKey, LogFile> LocalLogFilesMap;

  base::FilePath StartLocalLogFileInternal(int render_process_id, int lid);

  void StopLocalLogFileInternal(int render_process_id, int lid);

  void WriteToLocalLogFile(int render_process_id,
                           int lid,
                           const std::string& output,
                           base::OnceCallback<void(bool)> reply);
  LocalLogFilesMap::iterator CloseLocalLogFile(LocalLogFilesMap::iterator it);

  void PeerConnectionAddedInternal(int render_process_id,
                                   int lid,
                                   base::OnceCallback<void(bool)> reply);

  void PeerConnectionRemovedInternal(int render_process_id,
                                     int lid,
                                     base::OnceCallback<void(bool)> reply);

  void EnableLocalLoggingInternal(base::FilePath base_path,
                                  size_t max_file_size_bytes,
                                  base::OnceCallback<void(bool)> reply);

  void DisableLocalLoggingInternal(base::OnceCallback<void(bool)> reply);

  void SetLocalLogsObserverInternal(LocalLogsObserver* observer,
                                    base::OnceCallback<void()> reply);

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

  // TODO: !!!
  LocalLogsObserver* local_logs_observer_;

  // TODO: !!!
  base::FilePath local_logs_base_path_;

  // TODO: !!!
  size_t max_local_log_file_size_bytes_;

  // TODO: !!!
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRtcEventLogManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
