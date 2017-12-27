// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/prerender/prerender_url_loader_throttle.h"

#include "ipc/ipc_message.h"
#include "content/public/common/resource_request.h"
#include "net/base/load_flags.h"

namespace prerender {

PrerendererURLLoaderThrottle::PrerendererURLLoaderThrottle(
    int render_frame_id)
    : render_frame_id_(render_frame_id) {}

PrerendererURLLoaderThrottle::~PrerendererURLLoaderThrottle() {
}

void PrerendererURLLoaderThrottle::DetachFromCurrentSequence() {
  render_frame_id_ = MSG_ROUTING_NONE;

  // unregister from prerenderhelper
}

void PrerendererURLLoaderThrottle::WillStartRequest(
    content::ResourceRequest* request,
    bool* defer) {
// Priorities for prerendering requests are lowered, to avoid competing with
// other page loads, except on Android where this is less likely to be a
// problem. In some cases, this may negatively impact the performance of
// prerendering, see https://crbug.com/652746 for details.
#if !defined(OS_ANDROID)
  // Requests with the IGNORE_LIMITS flag set (i.e., sync XHRs)
  // should remain at MAXIMUM_PRIORITY.
  if (request->load_flags & net::LOAD_IGNORE_LIMITS) {
    DCHECK_EQ(request->priority, net::MAXIMUM_PRIORITY);
  } else if (request->priority != net::IDLE) {
    original_request_priority_ = request->priority;
    request->priority = net::IDLE;
  }
#endif  // OS_ANDROID
}

void PrerendererURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
}

void PrerendererURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    const content::ResourceResponseHead& response_head,
    bool* defer) {
}

}  // namespace prerender
