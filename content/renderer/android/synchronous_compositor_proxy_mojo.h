// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_MOJO_H_
#define CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_MOJO_H_

#include "content/renderer/android/synchronous_compositor_proxy.h"

namespace content {

// This class implements the SynchronousCompositorProxy with
// IPC messaging backed by mojo.
class SynchronousCompositorProxyMojo : public SynchronousCompositorProxy {
 public:
  SynchronousCompositorProxyMojo(
      ui::SynchronousInputHandlerProxy* input_handler_proxy);
  ~SynchronousCompositorProxyMojo() override;

  void SetNeedsBeginFrames(bool needs_begin_frames) override;

  void BindChannel(
      mojom::SynchronousCompositorControlHostPtr control_host,
      mojom::SynchronousCompositorHostAssociatedPtrInfo host,
      mojom::SynchronousCompositorAssociatedRequest compositor_request);

 protected:
  void SendAsyncRendererStateIfNeeded() override;
  void LayerTreeFrameSinkCreated() override;
  void SendBeginFrameResponse(
      const content::SyncCompositorCommonRendererParams&) override;
  void SendDemandDrawHwAsyncReply(
      const content::SyncCompositorCommonRendererParams&,
      uint32_t,
      base::Optional<viz::CompositorFrame>) override;

 private:
  mojom::SynchronousCompositorControlHostPtr control_host_;
  mojom::SynchronousCompositorHostAssociatedPtr host_;
  mojo::AssociatedBinding<mojom::SynchronousCompositor> binding_;
  bool needs_begin_frame_ = false;
  bool needs_layer_frame_sink_create_ = false;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorProxyMojo);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ANDROID_SYNCHRONOUS_COMPOSITOR_PROXY_MOJO_H_
