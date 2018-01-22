// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_remote_event_log_manager.h"

#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "content/public/browser/browser_context.h"

// TODO: !!! Retry limit?

namespace content {

namespace {
const base::TimeDelta kPendingLogFileRetention = base::TimeDelta::FromDays(3);
constexpr size_t kMaxPendingLogFilesPerBrowserContext = 10;  // TODO: !!! UT
}  // namespace

// TODO: !!! Block this for mobile, thereby making Android-specific values
// unnecessary.
const size_t kMaxNumberActiveRemoteWebRtcEventLogFiles = 5;

WebRtcRemoteEventLogManager::WebRtcRemoteEventLogManager(
    WebRtcRemoteEventLogsObserver* observer)
    : log_extension_(FILE_PATH_LITERAL("log")), observer_(observer) {}

// TODO: !!! Do we want to close anything? Or DCHECK that nothing is left open?
WebRtcRemoteEventLogManager::~WebRtcRemoteEventLogManager() = default;

void WebRtcRemoteEventLogManager::OnBrowserContextInitialized(
    const BrowserContext* browser_context) {
  if (!MaybeCreateLogsDirectory(browser_context)) {
    LOG(WARNING)
        << "WebRtcRemoteEventLogManager couldn't create logs directory.";
    return;
  }
  AddPendingLogs(browser_context);
}

bool WebRtcRemoteEventLogManager::PeerConnectionAdded(int render_process_id,
                                                      int lid) {
  const auto result = active_peer_connections_.emplace(render_process_id, lid);
  return result.second;
}

bool WebRtcRemoteEventLogManager::PeerConnectionRemoved(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context) {
  const PeerConnectionKey key = PeerConnectionKey(render_process_id, lid);
  auto peer_connection = active_peer_connections_.find(key);

  if (peer_connection == active_peer_connections_.end()) {
    return false;
  }

  MaybeStopRemoteLogging(render_process_id, lid, browser_context);

  active_peer_connections_.erase(peer_connection);

  return true;
}

bool WebRtcRemoteEventLogManager::StartRemoteLogging(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    size_t max_file_size_bytes) {
  const PeerConnectionKey key(render_process_id, lid);

  // May not start logging inactive peer connections.
  if (active_peer_connections_.find(key) == active_peer_connections_.end()) {
    return false;
  }

  // May not restart active remote logs.
  // TODO: !!! Unit test for different file size; original value still used.
  auto it = active_logs_.find(key);
  if (it != active_logs_.end()) {
    LOG(ERROR) << "Remote logging already underway for (" << render_process_id
               << ", " << lid << ").";
    return false;
  }

  // This is a good opportunity to prune the list of pending logs, potentially
  // making room for this file.
  PrunePendingLogs();

  // Limit over concurrently active logs (across BrowserContext-s).
  if (active_logs_.size() >= kMaxNumberActiveRemoteWebRtcEventLogFiles) {
    return false;
  }

  // Limit over the number of pending logs (per BrowserContext). We count active
  // logs too, since they become pending logs once completed.
  if (active_logs_counts_[browser_context] +
          pending_logs_counts_[browser_context] >=
      kMaxPendingLogFilesPerBrowserContext) {
    return false;
  }

  return StartWritingLog(render_process_id, lid, browser_context,
                         max_file_size_bytes);
}

// TODO: !!! Share this implementation between the local and remote. Update the
// error for that, though.
bool WebRtcRemoteEventLogManager::EventLogWrite(int render_process_id,
                                                int lid,
                                                const std::string& output) {
  DCHECK_LE(output.length(),
            static_cast<size_t>(std::numeric_limits<int>::max()));

  // Search for the associated active log.
  const PeerConnectionKey key(render_process_id, lid);
  auto it = active_logs_.find(key);
  if (it == active_logs_.end()) {
    return false;
  }

  // Observe the file size limit, if any. Note that base::File's interface does
  // not allow writing more than numeric_limits<int>::max() bytes at a time.
  int output_len = static_cast<int>(output.length());  // DCHECKed above.
  LogFile& log_file = it->second;
  if (log_file.max_file_size_bytes != kWebRtcEventLogManagerUnlimitedFileSize) {
    DCHECK_LT(log_file.file_size_bytes, log_file.max_file_size_bytes);
    if (log_file.file_size_bytes + output.length() < log_file.file_size_bytes ||
        log_file.file_size_bytes + output.length() >
            log_file.max_file_size_bytes) {
      output_len = log_file.max_file_size_bytes - log_file.file_size_bytes;
    }
  }

  int written = log_file.file.WriteAtCurrentPos(output.c_str(), output_len);
  if (written < 0 || written != output_len) {  // Error
    LOG(WARNING) << "WebRTC event log output couldn't be written to local "
                    "file in its entirety.";
    CloseLogFile(it);
    return false;
  }

  log_file.file_size_bytes += static_cast<size_t>(written);
  if (log_file.max_file_size_bytes != kWebRtcEventLogManagerUnlimitedFileSize) {
    DCHECK_LE(log_file.file_size_bytes, log_file.max_file_size_bytes);
    if (log_file.file_size_bytes >= log_file.max_file_size_bytes) {
      CloseLogFile(it);
    }
  }

  // Truncated output due to exceeding the maximum is reported as an error - the
  // caller is interested to know that not all of its output was written,
  // regardless of the reason.
  return (static_cast<size_t>(written) == output.length());
}

void WebRtcRemoteEventLogManager::OnWebRtcEventLogUploadSuccess() {
  // TODO: !!!
}

void WebRtcRemoteEventLogManager::OnWebRtcEventLogUploadFailure() {
  // TODO: !!!
}

base::FilePath WebRtcRemoteEventLogManager::GetLogsDirectoryPath(
    const BrowserContext* browser_context) {
  return browser_context->GetPath().Append("webrtc_event_logs");
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

void WebRtcRemoteEventLogManager::AddPendingLogs(
    const BrowserContext* browser_context) {
  DCHECK(active_logs_counts_.find(browser_context) ==
         active_logs_counts_.end());
  DCHECK(pending_logs_counts_.find(browser_context) ==
         pending_logs_counts_.end());

  active_logs_counts_.emplace(browser_context, 0);
  pending_logs_counts_.emplace(browser_context, 0);

  base::FileEnumerator enumerator(GetLogsDirectoryPath(browser_context),
                                  /*recursive=*/false,
                                  base::FileEnumerator::FILES,
                                  FILE_PATH_LITERAL("*.") + log_extension_);

  for (auto path = enumerator.Next(); !path.empty(); path = enumerator.Next()) {
    const auto last_modified = enumerator.GetInfo().GetLastModifiedTime();
    pending_logs_.emplace(browser_context, path, last_modified);
    pending_logs_counts_[browser_context] += 1;
  }

  PrunePendingLogs();

  MaybeStartUploading();
}

bool WebRtcRemoteEventLogManager::StartWritingLog(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    size_t max_file_size_bytes) {
  // Randomize a new filename. In the highly unlikely event that this filename
  // is already taken, it will be treated the same way as any other failure
  // to start the log file.
  // TODO: !!! Allow injecting the randomizer from unit tests, so that this
  // may be tested.
  const std::string unique_filename =
      "event_log_" + std::to_string(base::RandUint64());
  const base::FilePath base_path = GetLogsDirectoryPath(browser_context);
  const base::FilePath file_path =
      base_path.AppendASCII(unique_filename).AddExtension(log_extension_);

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
  const auto it = active_logs_.emplace(
      key, LogFile(file_path, std::move(file), max_file_size_bytes));
  DCHECK(it.second);
  active_logs_counts_[browser_context] += 1;

  observer_->OnRemoteLogStarted(PeerConnectionKey(render_process_id, lid),
                                file_path);

  return true;
}

void WebRtcRemoteEventLogManager::MaybeStopRemoteLogging(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context) {
  const PeerConnectionKey key(render_process_id, lid);
  const auto it = active_logs_.find(key);

  if (it == active_logs_.end()) {
    return;
  }

  it->second.file.Flush();
  it->second.file.Close();

  // The current time is a good enough approximation of the file's last
  // modification time.
  base::Time last_modified = base::Time::Now();

  pending_logs_.emplace(browser_context, it->second.path, last_modified);
  pending_logs_counts_[browser_context] += 1;

  active_logs_.erase(it);
  active_logs_counts_[browser_context] += 1;

  observer_->OnRemoteLogStopped(PeerConnectionKey(render_process_id, lid));

  MaybeStartUploading();
}

WebRtcRemoteEventLogManager::LogFilesMap::iterator
WebRtcRemoteEventLogManager::CloseLogFile(LogFilesMap::iterator it) {
  // const PeerConnectionKey peer_connection = it->first;

  it->second.file.Flush();
  it = active_logs_.erase(it);  // file.Close() called by destructor.

  // TODO: !!!
  // if (observer_) {
  //   observer_->OnLocalLogStopped(peer_connection);
  // }

  // TODO: !!! MaybeUpload

  return it;
}

void WebRtcRemoteEventLogManager::MaybeStartUploading() {
  if (uploader_) {
    return;  // Upload already underway.
  }

  if (pending_logs_.empty()) {
    return;  // Nothing to upload.
  }

  uploader_ = std::make_unique<WebRtcEventLogUploader>(
      this, pending_logs_.begin()->path);

  // No longer pending. (If the upload fails, the log will be deleted.)
  // TODO(eladalon): Add more refined retry behavior, so that we would not
  // delete the log permanently if the network is just down, on the one hand,
  // but also would not be uploading unlimited data on endless retries on the
  // other hand. https://crbug.com/775415
  pending_logs_.erase(pending_logs_.begin());
}

void WebRtcRemoteEventLogManager::PrunePendingLogs() {
  const base::Time oldest_non_expired_timestamp =
      base::Time::Now() - kPendingLogFileRetention;
  for (auto it = pending_logs_.begin(); it != pending_logs_.end();) {
    if (it->last_modified < oldest_non_expired_timestamp) {
      if (!base::DeleteFile(it->path, /*recursive=*/false)) {
        LOG(ERROR) << "Failed to delete " << it->path << ".";
      }
      pending_logs_counts_[it->browser_context] -= 1;
      it = pending_logs_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace content
