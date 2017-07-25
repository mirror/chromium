// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_MOJO_FRAME_HOST_FORWARDER_H_
#define CONTENT_BROWSER_FRAME_HOST_MOJO_FRAME_HOST_FORWARDER_H_

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/common/frame.mojom.h"

namespace content {

// Implementation of mojom::FrameHost that simply delegates all
// incoming calls to |receiver|, which must outlive this in instance.
//
// The purpose of this class is to allow intercepting these calls in tests.
class CONTENT_EXPORT MojoFrameHostForwarder : public mojom::FrameHost {
 public:
  // A non-null |receiver| must be set during times when a call is forwarded.
  // This is enforced by DCHECKs, so spurious calls are not silently dropped.
  explicit MojoFrameHostForwarder(mojom::FrameHost* receiver = nullptr);
  ~MojoFrameHostForwarder() override;

  void set_receiver(mojom::FrameHost* receiver) { receiver_ = receiver; }

 protected:
  // mojom::FrameHost:
  bool CreateNewWindow(mojom::CreateNewWindowParamsPtr params,
                       mojom::CreateNewWindowReplyPtr* reply) override;
  void CreateNewWindow(mojom::CreateNewWindowParamsPtr params,
                       CreateNewWindowCallback callback) override;
  void DidCommitProvisionalLoad(
      const ::FrameHostMsg_DidCommitProvisionalLoad_Params& params) override;
  void BindInterfaceProviderForNewDocument(
      ::service_manager::mojom::InterfaceProviderRequest request) override;

 private:
  mojom::FrameHost* receiver_;

  DISALLOW_COPY_AND_ASSIGN(MojoFrameHostForwarder);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_MOJO_FRAME_HOST_FORWARDER_H_
