// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/did_commit_provisional_load_interceptor.h"

#include "base/logging.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

DidCommitProvisionalLoadInterceptor::DidCommitProvisionalLoadInterceptor(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host)
    : WebContentsObserver(web_contents), render_frame_host_(render_frame_host) {
  DCHECK(render_frame_host_);
  auto* rfhi = static_cast<RenderFrameHostImpl*>(render_frame_host_);
  MojoFrameHostForwarder::set_receiver(rfhi);
  rfhi->override_mojo_frame_host_receiver_for_testing(this);
}

DidCommitProvisionalLoadInterceptor::~DidCommitProvisionalLoadInterceptor() {
  if (render_frame_host_)
    StopIntercepting();
}

void DidCommitProvisionalLoadInterceptor::StopIntercepting() {
  DCHECK(render_frame_host_);
  auto* rfhi = static_cast<RenderFrameHostImpl*>(render_frame_host_);
  MojoFrameHostForwarder::set_receiver(nullptr);
  rfhi->override_mojo_frame_host_receiver_for_testing(nullptr);
  render_frame_host_ = nullptr;
}

void DidCommitProvisionalLoadInterceptor::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  if (render_frame_host == render_frame_host_)
    StopIntercepting();
}

}  // namespace content
