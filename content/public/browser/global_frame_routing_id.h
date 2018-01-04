// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_GLOBAL_FRAME_ROUTING_ID_H_
#define CONTENT_PUBLIC_BROWSER_GLOBAL_FRAME_ROUTING_ID_H_

#include <tuple>

#include "ipc/ipc_message.h"

namespace content {

// Uniquely identifies the route between a render frame host and its render
// frame. The frame_route_id must be a RenderFrameHost routing id.
struct GlobalFrameRoutingId {
  GlobalFrameRoutingId() : child_id(0), frame_routing_id(MSG_ROUTING_NONE) {}

  GlobalFrameRoutingId(int child_id, int frame_routing_id)
      : child_id(child_id), frame_routing_id(frame_routing_id) {}

  // The unique ID of the child process (different from OS's PID).
  int child_id;

  // The route ID (unique for each URLRequest source).
  int frame_routing_id;

  bool operator<(const GlobalFrameRoutingId& other) const {
    return std::tie(child_id, frame_routing_id) <
           std::tie(other.child_id, other.frame_routing_id);
  }
  bool operator==(const GlobalFrameRoutingId& other) const {
    return child_id == other.child_id &&
           frame_routing_id == other.frame_routing_id;
  }
  bool operator!=(const GlobalFrameRoutingId& other) const {
    return !(*this == other);
  }
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_GLOBAL_FRAME_ROUTING_ID_H_
