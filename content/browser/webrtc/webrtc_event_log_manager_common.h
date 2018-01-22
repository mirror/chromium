// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO: !!! All of these.
#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_COMMON_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_COMMON_H_

#include <tuple>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "build/build_config.h"
#include "content/common/content_export.h"

namespace content {

CONTENT_EXPORT extern const size_t kWebRtcEventLogManagerUnlimitedFileSize;

CONTENT_EXPORT extern const size_t kDefaultMaxLocalLogFileSizeBytes;
CONTENT_EXPORT extern const size_t kMaxNumberLocalWebRtcEventLogFiles;

// For a given Chrome session, this is a unique key for PeerConnections.
// It's not, however, unique between sessions (after Chrome is restarted).
struct WebRtcEventLogPeerConnectionKey {
  constexpr WebRtcEventLogPeerConnectionKey(int render_process_id, int lid)
      : render_process_id(render_process_id), lid(lid) {}

  bool operator==(const WebRtcEventLogPeerConnectionKey& other) const {
    return std::tie(render_process_id, lid) ==
           std::tie(other.render_process_id, other.lid);
  }

  bool operator<(const WebRtcEventLogPeerConnectionKey& other) const {
    return std::tie(render_process_id, lid) <
           std::tie(other.render_process_id, other.lid);
  }

  int render_process_id;
  int lid;  // Renderer-local PeerConnection ID.
};

// An observer for notifications of local log files being started/stopped, and
// the paths which will be used for these logs.
class WebRtcLocalEventLogsObserver {
 public:
  virtual ~WebRtcLocalEventLogsObserver() = default;
  virtual void OnLocalLogStarted(WebRtcEventLogPeerConnectionKey key,
                                 base::FilePath file_path) = 0;
  virtual void OnLocalLogStopped(WebRtcEventLogPeerConnectionKey key) = 0;
};

// An observer for notifications of remote-bound log files being
// started/stopped. The start event would likely only interest unit tests
// (because it exposes the randomized filename to them). The stop event is of
// general interest, because it would often mean that WebRTC can stop sending
// us event logs for this peer connection.
// Some cases where OnRemoteLogStopped would be called include::
// 1. The PeerConnection has become inactive.
// 2. The file's maximum size has been reached.
// 3. Any type of error while writing to the file.
class WebRtcRemoteEventLogsObserver {
 public:
  virtual ~WebRtcRemoteEventLogsObserver() = default;
  virtual void OnRemoteLogStarted(WebRtcEventLogPeerConnectionKey pc_key,
                                  base::FilePath file_path) = 0;
  virtual void OnRemoteLogStopped(WebRtcEventLogPeerConnectionKey pc_key) = 0;
};

struct LogFile {
  LogFile(base::FilePath path, base::File file, size_t max_file_size_bytes)
      : path(path),
        file(std::move(file)),
        max_file_size_bytes(max_file_size_bytes),
        file_size_bytes(0) {}
  const base::FilePath path;
  base::File file;
  const size_t max_file_size_bytes;
  size_t file_size_bytes;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_COMMON_H_
