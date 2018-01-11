// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_

#include <map>
#include <queue>
#include <set>
#include <vector>

#include "base/optional.h"  // TODO: !!! Used?
#include "base/time/time.h"
#include "content/browser/webrtc/webrtc_event_log_manager_common.h"
#include "content/public/browser/browser_context.h"

namespace content {

class WebRtcRemoteEventLogManager final {
 public:
  WebRtcRemoteEventLogManager();
  ~WebRtcRemoteEventLogManager();

  void OnBrowserContextInitialized(const BrowserContext* browser_context);

  // TODO: !!!
  bool PeerConnectionAdded(int render_process_id, int lid);
  bool PeerConnectionRemoved(int render_process_id, int lid);

  // TODO: !!! 1. Document. 2. Is "remote: superfluous here?
  bool StartRemoteLogging(int render_process_id,
                          int lid,
                          const BrowserContext* browser_context,
                          size_t max_file_size_bytes);

  bool EventLogWrite(int render_process_id, int lid, const std::string& output);

  static const size_t kMaxConcurrentActiveLogs;
  static const size_t kMaxLogsPendingUpload;
  // TODO: !!! Max retention.

 private:
  using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;

  struct PendingLog {
    PendingLog(base::FilePath path, base::Time last_modified)
        : path(path), last_modified(last_modified) {}

    bool operator<(const PendingLog& other) const {
      return last_modified < other.last_modified;
    }

    base::FilePath path;
    base::Time last_modified;
  };

  // Attempts to create the directory where we'll write the logs, if it does
  // not already exist. Returns true if the directory exists (either it already
  // existed, or it was successfully created);
  bool MaybeCreateLogsDirectory(const BrowserContext* browser_context);

  // TODO: !!! 1. Comment. 2. Mention how this prunes old logs.
  void AddPendingLogs(const BrowserContext* browser_context);

  // TODO: !!!
  bool MaybeStartWritingLog(int render_process_id,
                            int lid,
                            const BrowserContext* browser_context,
                            size_t max_file_size_bytes);

  // TODO: !!!
//  void PrunePendingLogsForBrowserContext(const BrowserContext* browser_context);

  // TODO: !!!
  bool PrunePendingLogsAndCheckIfNewLogAllowed();  // TODO: !!!!!!

  const base::FilePath::StringType log_extension_;

//  const BrowserContext* const browser_context_;

  // Currently active peer connections. PeerConnections which have been closed
  // are not considered active, regardless of whether they have been torn down.
  std::set<PeerConnectionKey> active_peer_connections_;

  // Remote-bound logs which we're currently in the process of writing to disk.
  std::map<PeerConnectionKey, LogFile> active_remote_logs_;

  // TODO: !!! 1. Comment. 2. Test that this is the right order.
  std::priority_queue<PendingLog> pending_logs_;

  std::map<const BrowserContext*, size_t> pending_logs_counts_;

  // List of files, from oldest to newest, that are pending upload.
  //  std::vector<base::FilePath> logs_pending_upload_;  // TODO: !!!

  DISALLOW_COPY_AND_ASSIGN(WebRtcRemoteEventLogManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
