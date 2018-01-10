// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_remote_event_log_manager.h"

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/rand_util.h"

namespace content {

base::FilePath GetLogsDirectoryPath(const BrowserContext* browser_context) {
  return browser_context->GetPath().Append("webrtc_event_logs");
}

const size_t WebRtcRemoteEventLogManager::kMaxConcurrentActiveLogs = 3;
const size_t WebRtcRemoteEventLogManager::kMaxLogsPendingUpload = 10;

WebRtcRemoteEventLogManager::WebRtcRemoteEventLogManager() {
  // TODO: !!!
}

WebRtcRemoteEventLogManager::~WebRtcRemoteEventLogManager() {
  // TODO: !!!
}

void WebRtcRemoteEventLogManager::OnBrowserContextInitialized(
    const BrowserContext* browser_context) {
  if (!MaybeCreateLogsDirectory(browser_context)) {
    LOG(WARNING) << "WebRtcRemoteEventLogManager couldn't create logs directory.";
    return;
  }

  // TODO: !!! Read the directory, and if it has any pending logs, add
  // them to the list of pending logs.
}

bool WebRtcRemoteEventLogManager::PeerConnectionAdded(int render_process_id,
                                                      int lid) {
  const auto result = active_peer_connections_.emplace(render_process_id, lid);
  return result.second;
}

bool WebRtcRemoteEventLogManager::PeerConnectionRemoved(int render_process_id,
                                                        int lid) {
  const PeerConnectionKey key = PeerConnectionKey(render_process_id, lid);
  auto peer_connection = active_peer_connections_.find(key);

  if (peer_connection == active_peer_connections_.end()) {
    return false;
  }

  // TODO: !!! 1. Finish logs which are being written and make them
  // pending logs. (Also unit test this.)
  // 2. WebRTC might need to be notified.

  active_peer_connections_.erase(peer_connection);

  return true;
}

bool WebRtcRemoteEventLogManager::StartRemoteLogging(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    size_t max_file_size_bytes) {
  // May NOT restart remote logging NOR use this interface for
  // updating the maximum file size.
  // TODO: !!! Unit test for different file size; original value still used.
  auto it = active_remote_logs_.find(PeerConnectionKey(render_process_id, lid));
  if (it != active_remote_logs_.end()) {
    LOG(ERROR) << "Remote logging already underway for (" << render_process_id
               << ", " << lid << ").";
    return false;
  }

  // TODO: !!! Already pruned?
  if (active_remote_logs_.size() >= kMaxConcurrentActiveLogs) {
    return false;
  }

  return MaybeStartWritingLogToDisk(render_process_id, lid, browser_context,
                                    max_file_size_bytes);
}

bool WebRtcRemoteEventLogManager::EventLogWrite(int render_process_id, int lid, const std::string& output) {
  // TODO: !!!
  return true;
}

bool WebRtcRemoteEventLogManager::MaybeCreateLogsDirectory(
    const BrowserContext* browser_context) {
  const base::FilePath path = GetLogsDirectoryPath(browser_context);

  if (base::PathExists(path)) {
    if (!base::DirectoryExists(path)) {
      LOG(ERROR) << "Path for remote-bound logs is taken by a non-directory.";
      return false;
    }
    // TODO: !!! Permissions too?
    return true;  // Directory already exists.
  }

  if (!base::CreateDirectory(path)) {
    LOG(ERROR) << "Failed to create the local directory for remote-bound logs.";
    return false;
  }

  return true;  // Directory successfully created.
}

bool WebRtcRemoteEventLogManager::MaybeStartWritingLogToDisk(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    size_t max_file_size_bytes) {
  // Observe the limit over the number of pending logs.
  if (!PrunePendingLogsAndCheckIfNewLogAllowed()) {
    return false;
  }

  // Randomize a new filename. In the highly unlikely event that this filename
  // is already taken, it will be treated the same way as any other failure
  // to start the log file.
  // TODO: !!! Allow injecting the randomizer from unit tests, so that this
  // may be tested.
  const std::string unique_filename = base::RandBytesAsString(8);
  const base::FilePath base_path = GetLogsDirectoryPath(browser_context);
  const base::FilePath file_path = base_path.AppendASCII(unique_filename)
                                       .AddExtension(FILE_PATH_LITERAL("log"));

  // Attempt to create the file.
  constexpr int file_flags = base::File::FLAG_CREATE | base::File::FLAG_WRITE |
                             base::File::FLAG_EXCLUSIVE_WRITE;
  base::File file(file_path, file_flags);
  if (!file.IsValid() || !file.created()) {
    LOG(WARNING) << "Couldn't create and/or open remote WebRTC event log file.";
    return false;
  }

  // Record that we're now writing this remote-bound log to this file.
  const PeerConnectionKey key(render_process_id, lid);
  const auto it = active_remote_logs_.emplace(
      key, LogFile(std::move(file), max_file_size_bytes));
  DCHECK(it.second);

  return true;
}

bool WebRtcRemoteEventLogManager::PrunePendingLogsAndCheckIfNewLogAllowed() {
  // TODO: !!! kMaxLogsPendingUpload

  // TODO: !!! Don't forget to avoid pruning files that are
  // currently being uploaded.
  return true;
}

}  // namespace content
