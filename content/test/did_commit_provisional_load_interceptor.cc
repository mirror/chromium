// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/did_commit_provisional_load_interceptor.h"

#include <tuple>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

namespace {

class MojoFrameHostInterceptor : public MojoFrameHostForwarder {
 public:
  // The |delegate| and |render_frame_host| must outlive this instance.
  explicit MojoFrameHostInterceptor(
      DidCommitProvisionalLoadInterceptor* delegate,
      RenderFrameHostImpl* render_frame_host)
      : MojoFrameHostForwarder(render_frame_host),
        delegate_(delegate),
        render_frame_host_(render_frame_host) {
    render_frame_host_->override_mojo_frame_host_receiver_for_testing(this);
  }

  ~MojoFrameHostInterceptor() override {
    render_frame_host_->override_mojo_frame_host_receiver_for_testing(nullptr);
  }

 protected:
  // MojoFrameHostForwarder:
  void DidCommitProvisionalLoad(
      mojom::DidCommitProvisionalLoadParamsPtr params) override {
    delegate_->WillDispatchDidCommitProvisionalLoad(render_frame_host_,
                                                    params->native_params);

    // Call base class implementation to deliver the message to the RFHI.
    MojoFrameHostForwarder::DidCommitProvisionalLoad(std::move(params));
  }

 private:
  DidCommitProvisionalLoadInterceptor* delegate_;
  RenderFrameHostImpl* render_frame_host_;

  DISALLOW_COPY_AND_ASSIGN(MojoFrameHostInterceptor);
};

}  // namespace

DidCommitProvisionalLoadInterceptor::DidCommitProvisionalLoadInterceptor(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  StartIntercepting(web_contents->GetMainFrame());
}

DidCommitProvisionalLoadInterceptor::~DidCommitProvisionalLoadInterceptor() {}

void DidCommitProvisionalLoadInterceptor::StartIntercepting(
    RenderFrameHost* render_frame_host) {
  bool did_insert;
  std::tie(std::ignore, did_insert) = forwarders_.emplace(
      render_frame_host,
      base::MakeUnique<MojoFrameHostInterceptor>(
          this, static_cast<RenderFrameHostImpl*>(render_frame_host)));
  DCHECK(did_insert);
}

void DidCommitProvisionalLoadInterceptor::RenderFrameCreated(
    RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetParent())
    return;
  StartIntercepting(render_frame_host);
}

void DidCommitProvisionalLoadInterceptor::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetParent())
    return;
  bool did_remove = !!forwarders_.erase(render_frame_host);
  DCHECK(did_remove);
}

}  // namespace content
