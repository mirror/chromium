// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_

#include <map>
#include <set>
#include <vector>

#include "base/time/time.h"
#include "content/browser/webrtc/webrtc_event_log_manager_common.h"
#include "content/browser/webrtc/webrtc_event_log_uploader.h"
#include "content/common/content_export.h"

// TODO: !!! Order .h and .cc files.

// TODO: !!! In the future, we want to be able to pause logs. Write comment.

namespace content {

class BrowserContext;

class CONTENT_EXPORT WebRtcRemoteEventLogManager final
    : public WebRtcEventLogUploaderObserver {
 public:
  static const size_t kMaxConcurrentActiveLogs;
  // TODO: !!! Max retention.

  explicit WebRtcRemoteEventLogManager(WebRtcRemoteEventLogsObserver* observer);
  ~WebRtcRemoteEventLogManager() override;

  void OnBrowserContextInitialized(const BrowserContext* browser_context);

  // TODO: !!!
  bool PeerConnectionAdded(int render_process_id, int lid);
  bool PeerConnectionRemoved(int render_process_id,
                             int lid,
                             const BrowserContext* browser_context);

  // TODO: !!! Document.
  bool StartRemoteLogging(int render_process_id,
                          int lid,
                          const BrowserContext* browser_context,
                          size_t max_file_size_bytes);

  bool EventLogWrite(int render_process_id, int lid, const std::string& output);

  // WebRtcEventLogUploaderObserver implementation.
  void OnWebRtcEventLogUploadSuccess() override;
  void OnWebRtcEventLogUploadFailure() override;

 protected:
  friend class WebRtcEventLogManagerTest;

  static base::FilePath GetLogsDirectoryPath(
      const BrowserContext* browser_context);

 private:
  using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;

  struct PendingLog {
    PendingLog(const BrowserContext* browser_context,
               base::FilePath path,
               base::Time last_modified)
        : browser_context(browser_context),
          path(path),
          last_modified(last_modified) {}

    bool operator<(const PendingLog& other) const {
      return last_modified < other.last_modified;
    }

    const BrowserContext* const browser_context;  // This file's owner.
    const base::FilePath path;
    // |last_modified| recorded at BrowserContext initialization. Chrome will
    // not modify it afterwards, and neither should the user.
    const base::Time last_modified;
  };

  // Attempts to create the directory where we'll write the logs, if it does
  // not already exist. Returns true if the directory exists (either it already
  // existed, or it was successfully created);
  bool MaybeCreateLogsDirectory(const BrowserContext* browser_context);

  // TODO: !!! 1. Comment. 2. Mention how this prunes old logs.
  void AddPendingLogs(const BrowserContext* browser_context);

  bool StartWritingLog(int render_process_id,
                       int lid,
                       const BrowserContext* browser_context,
                       size_t max_file_size_bytes);

  void MaybeStopRemoteLogging(int render_process_id,
                              int lid,
                              const BrowserContext* browser_context);

  void MaybeStartUploading();

  void PrunePendingLogs();

  const base::FilePath::StringType log_extension_;

  WebRtcRemoteEventLogsObserver* const observer_;

  // Currently active peer connections. PeerConnections which have been closed
  // are not considered active, regardless of whether they have been torn down.
  std::set<PeerConnectionKey> active_peer_connections_;

  // Remote-bound logs which we're currently in the process of writing to disk.
  std::map<PeerConnectionKey, LogFile> active_logs_;

  // TODO: !!! 1. Comment. 2. Test that this is the right order.
  // TODO: !!! Remember that the first one is not necessarily the one which
  // is being uploaded.
  std::set<PendingLog> pending_logs_;

  std::unique_ptr<WebRtcEventLogUploader> uploader_;

  // TODO: !!!
  std::map<const BrowserContext*, size_t> active_logs_counts_;
  std::map<const BrowserContext*, size_t> pending_logs_counts_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcRemoteEventLogManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
