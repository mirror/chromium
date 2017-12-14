// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_remote_event_log_manager.h"

namespace content {

WebRtcRemoteEventLogManager::WebRtcRemoteEventLogManager() {}

WebRtcRemoteEventLogManager::~WebRtcRemoteEventLogManager() {}

bool WebRtcRemoteEventLogManager::PeerConnectionAdded(int render_process_id,
                                                      int lid) {
  const auto result = active_peer_connections_.emplace(render_process_id, lid);

  if (!result.second) {  // PeerConnection already registered.
    return false;
  }

  // TODO: !!!

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

}  // namespace content
