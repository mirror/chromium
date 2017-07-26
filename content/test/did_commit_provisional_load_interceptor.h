// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
#define CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "content/browser/frame_host/mojo_frame_host_forwarder.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class WebContents;
class RenderFrameHost;

// Allows intercepting calls to mojom::FrameHost::DidCommitProvisionalLoad
// before they are dispatched to RFHIs, enabling unit/browser tests to
// scrutinize the parameters, or trigger other calls just before the navigation
// is committed to simulate race conditions.
class DidCommitProvisionalLoadInterceptor : public WebContentsObserver {
 public:
  // Constructs an instance that will intercept DidCommitProvisionalLoad calls
  // corresponding to main frame navigations in |web_contents| while the
  // instance is in scope.
  explicit DidCommitProvisionalLoadInterceptor(WebContents* web_contents);
  ~DidCommitProvisionalLoadInterceptor() override;

  // Called just before DidCommitProvisionalLoad with |params| would be
  // dispatched to a main frame, active/provisional/speculative RFH.
  virtual void WillDispatchDidCommitProvisionalLoad(
      RenderFrameHost* render_frame_host,
      const ::FrameHostMsg_DidCommitProvisionalLoad_Params& params) = 0;

 private:
  void StartIntercepting(RenderFrameHost* render_frame_host);
  void StopIntercepting(RenderFrameHost* render_frame_host);

  // WebContentsObserver:
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;

  // Owns forwarders that intercept mojom::FrameHost calls to the active /
  // provisional / speculative main-frame RenderFrameHost in this WebContents.
  using HostForwarderMap =
      std::map<RenderFrameHost*, std::unique_ptr<MojoFrameHostForwarder>>;
  HostForwarderMap forwarders_;

  DISALLOW_COPY_AND_ASSIGN(DidCommitProvisionalLoadInterceptor);
};

}  // namespace content

#endif  // CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
