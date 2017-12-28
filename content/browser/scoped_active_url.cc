// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/scoped_active_url.h"

#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

ScopedActiveURL::ScopedActiveURL(const GURL& active_url,
                                 const url::Origin& top_origin) {
  GetContentClient()->SetActiveURL(active_url, top_origin.Serialize());
}

ScopedActiveURL::ScopedActiveURL(RenderFrameHost* frame)
    : ScopedActiveURL(frame->GetLastCommittedURL(), GetTopOrigin(frame)) {}

ScopedActiveURL::~ScopedActiveURL() {
  GetContentClient()->SetActiveURL(GURL(), "");
}

// static
const url::Origin& ScopedActiveURL::GetTopOrigin(RenderFrameHost* frame) {
  while (frame->GetParent())
    frame = frame->GetParent();
  return frame->GetLastCommittedOrigin();
}

}  // namespace content
