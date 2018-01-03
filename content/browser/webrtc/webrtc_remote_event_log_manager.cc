// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_remote_event_log_manager.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"

// TODO: !!!
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"

namespace content {

WebRtcRemoteEventLogManager::WebRtcRemoteEventLogManager()
    : remote_log_path_(
          GetContentClient()->browser()->GetDefaultDownloadDirectory().Append(
              FILE_PATH_LITERAL("REMOTE"))) {
  // TODO: !!!
}

WebRtcRemoteEventLogManager::~WebRtcRemoteEventLogManager() {
  // This should never actually run, except in unit tests.
}

void WebRtcRemoteEventLogManager::Init() {
  if (!MaybeCreateLogsDirectory()) {
    return;
  }
}

bool WebRtcRemoteEventLogManager::PeerConnectionAdded(int render_process_id,
                                                      int lid) {
  const auto result = active_peer_connections_.emplace(render_process_id, lid);

  if (!result.second) {  // PeerConnection already registered.
    return false;
  }

  return true;
}

bool WebRtcRemoteEventLogManager::PeerConnectionRemoved(int render_process_id,
                                                        int lid) {
  const PeerConnectionKey key = PeerConnectionKey(render_process_id, lid);
  auto peer_connection = active_peer_connections_.find(key);

  if (peer_connection == active_peer_connections_.end()) {
    return false;
  }

  // TODO: !!!

  active_peer_connections_.erase(peer_connection);

  return true;
}

bool WebRtcRemoteEventLogManager::StartRemoteLogging(
    int render_process_id,
    int lid,
    size_t max_file_size_bytes) {
  auto it = active_remote_logs_.find(PeerConnectionKey(render_process_id, lid));
  if (it != active_remote_logs_.end()) {
    // Log already started. Note: Changing of max size not allowed.
    LOG(ERROR) << "Remote logging already underway for (" << render_process_id
               << ", " << lid << ").";
    return false;
  }

  if (!MaybeCreateLogsDirectory()) {
    return false;
  }
  // TODO: !!!
  return true;
}

bool WebRtcRemoteEventLogManager::MaybeCreateLogsDirectory() {
  if (base::PathExists(remote_log_path_)) {
    if (!base::DirectoryExists(remote_log_path_)) {
      LOG(ERROR) << "Path for remote-bound logs is taken by a non-directory.";
      return false;
    }
    // TODO: !!! Permissions too?
    return true;  // Directory already exists.
  }

  if (!base::CreateDirectory(remote_log_path_)) {
    LOG(ERROR) << "Failed to create the local directory for remote-bound logs.";
    return false;
  }

  return true;  // Directory successfully created.
}

bool WebRtcRemoteEventLogManager::MaybeStartWritingToDisk() {
  return true;  // TODO: !!!!!!!!
}

}  // namespace content
