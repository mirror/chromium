// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_

#include <map>
#include <set>
#include <vector>

#include "base/optional.h"
#include "content/browser/webrtc/webrtc_event_log_manager_common.h"
#include "content/public/browser/browser_context.h"

namespace content {

class WebRtcRemoteEventLogManager final {
 public:
  explicit WebRtcRemoteEventLogManager(const BrowserContext* browser_context);
  ~WebRtcRemoteEventLogManager();

  // TODO: !!!
  void Init();

  // TODO: !!!
  bool PeerConnectionAdded(int render_process_id, int lid);
  bool PeerConnectionRemoved(int render_process_id, int lid);

  // TODO: !!!
  bool StartRemoteLogging(int render_process_id,
                          int lid,
                          size_t max_file_size_bytes);

  static const size_t kMaxConcurrentActiveLogs;
  static const size_t kMaxLogsPendingUpload;
  // TODO: !!! Max retention.

 private:
  using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;

  // Attempts to create the directory for the local logs, if it does not already
  // exist. Returns true if the directory either existed before, or was
  // successfully created.
  bool MaybeCreateLogsDirectory();

  // TODO: !!!
  bool MaybeStartWritingToDisk(int render_process_id,
                               int lid,
                               size_t max_file_size_bytes);

  // TODO: !!!
  bool PrunePendingLogsAndCheckIfNewLogAllowed();

//  const BrowserContext* const browser_context_;

  // The path where remote-bound logs should be stored on disk until they
  // get uploaded.
  const base::FilePath remote_log_path_;

  // Currently active peer connections. PeerConnections which have been closed
  // are not considered active, regardless of whether they have been torn down.
  std::set<PeerConnectionKey> active_peer_connections_;

  // Remote-bound logs which we're currently in the process of writing to disk.
  std::map<PeerConnectionKey, LogFile> active_remote_logs_;

  // List of files, from oldest to newest, that are pending upload.
  //  std::vector<base::FilePath> logs_pending_upload_;  // TODO: !!!

  DISALLOW_COPY_AND_ASSIGN(WebRtcRemoteEventLogManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
