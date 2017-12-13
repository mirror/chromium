// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_COMMON_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_COMMON_H_

#include <tuple>

#include "base/files/file_path.h"
#include "build/build_config.h"

namespace content {

constexpr size_t kWebRtcEventLogManagerUnlimitedFileSize = 0;

#if defined(OS_ANDROID)
constexpr size_t kMaxNumberLocalWebRtcEventLogFiles = 3;
constexpr size_t kDefaultMaxLocalLogFileSizeBytes = 10000000;
#else
constexpr size_t kMaxNumberLocalWebRtcEventLogFiles = 5;
constexpr size_t kDefaultMaxLocalLogFileSizeBytes = 60000000;
#endif

// For a given Chrome session, this is a unique key for PeerConnections.
// It's not, however, unique between sessions (after Chrome is restarted).
struct PeerConnectionKey {
  constexpr PeerConnectionKey(int render_process_id, int lid)
      : render_process_id(render_process_id), lid(lid) {}

  bool operator==(const PeerConnectionKey& other) const {
    return std::tie(render_process_id, lid) ==
           std::tie(other.render_process_id, other.lid);
  }

  bool operator<(const PeerConnectionKey& other) const {
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
  virtual void OnLocalLogStarted(PeerConnectionKey peer_connection,
                                 base::FilePath file_path) = 0;
  virtual void OnLocalLogStopped(PeerConnectionKey peer_connection) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_COMMON_H_
