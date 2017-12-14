// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_

#include <set>

#include "content/browser/webrtc/webrtc_event_log_manager_common.h"

namespace content {

class WebRtcRemoteEventLogManager {
 public:
  WebRtcRemoteEventLogManager();
  ~WebRtcRemoteEventLogManager();

  bool PeerConnectionAdded(int render_process_id, int lid);
  bool PeerConnectionRemoved(int render_process_id, int lid);

 private:
  using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;

  // Currently active peer connections. PeerConnections which have been closed
  // are not considered active, regardless of whether they have been torn down.
  std::set<PeerConnectionKey> active_peer_connections_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcRemoteEventLogManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_REMOTE_EVENT_LOG_MANAGER_H_
