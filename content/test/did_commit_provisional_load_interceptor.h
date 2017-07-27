// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
#define CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/frame.mojom.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class WebContents;
class RenderFrameHost;

// Allows intercepting calls to RFHI::DidCommitProvisionalLoad before just
// before they would be processed, enabling unit/browser tests to scrutinize the
// parameters, or trigger other calls just before the navigation is committed to
// simulate race conditions.
class DidCommitProvisionalLoadInterceptor
    : public WebContentsObserver,
      public base::SupportsWeakPtr<DidCommitProvisionalLoadInterceptor> {
 public:
  // Constructs an instance that will intercept DidCommitProvisionalLoad calls
  // corresponding to main frame navigations in |web_contents| while the
  // instance is in scope.
  explicit DidCommitProvisionalLoadInterceptor(WebContents* web_contents);
  ~DidCommitProvisionalLoadInterceptor() override;

  // Called just before DidCommitProvisionalLoad with |params| would be
  // dispatched to a main frame RFH, be it active/provisional/speculative.
  virtual void WillDispatchDidCommitProvisionalLoad(
      RenderFrameHost* render_frame_host,
      const ::FrameHostMsg_DidCommitProvisionalLoad_Params& params) = 0;

 private:
  void StartIntercepting(RenderFrameHost* render_frame_host);

  // WebContentsObserver:
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;

  DISALLOW_COPY_AND_ASSIGN(DidCommitProvisionalLoadInterceptor);
};

}  // namespace content

#endif  // CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
