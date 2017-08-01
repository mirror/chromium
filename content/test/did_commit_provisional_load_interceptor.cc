// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/did_commit_provisional_load_interceptor.h"

#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

DidCommitProvisionalLoadInterceptor::DidCommitProvisionalLoadInterceptor(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  StartIntercepting(web_contents->GetMainFrame());
}

DidCommitProvisionalLoadInterceptor::~DidCommitProvisionalLoadInterceptor() =
    default;

void DidCommitProvisionalLoadInterceptor::StartIntercepting(
    RenderFrameHost* render_frame_host) {
  static_cast<RenderFrameHostImpl*>(render_frame_host)
      ->set_did_commit_provisional_load_interceptor_for_testing(
          base::BindRepeating(&DidCommitProvisionalLoadInterceptor::
                                  WillDispatchDidCommitProvisionalLoad,
                              AsWeakPtr(), render_frame_host));
}

void DidCommitProvisionalLoadInterceptor::RenderFrameCreated(
    RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetParent())
    return;
  StartIntercepting(render_frame_host);
}

}  // namespace content
