// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
#define CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_

#include "base/macros.h"
#include "content/browser/frame_host/mojo_frame_host_forwarder.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class WebContents;
class RenderFrameHost;

class DidCommitProvisionalLoadInterceptor : public WebContentsObserver,
                                            public MojoFrameHostForwarder {
 public:
  explicit DidCommitProvisionalLoadInterceptor(
      WebContents* web_contents,
      RenderFrameHost* render_frame_host);
  ~DidCommitProvisionalLoadInterceptor() override;

 protected:
  // Removes to override on the RFHI that redirects calls to |this|, and also
  // resets the target where the MojoFrameHostForwarder forwards.
  void StopIntercepting();

  // WebContentsObserver:
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;

 private:
  // The RFH for which |this| is intercepting mojom::FrameHost calls, or nullptr
  // if the RFH has already been destroyed.
  RenderFrameHost* render_frame_host_;

  DISALLOW_COPY_AND_ASSIGN(DidCommitProvisionalLoadInterceptor);
};

}  // namespace content

#endif  // CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
