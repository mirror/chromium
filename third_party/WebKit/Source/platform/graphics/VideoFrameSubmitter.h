// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/thread_checker.h"
#include "components/viz/common/surfaces/local_surface_id_allocator.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/PlatformExport.h"
#include "platform/graphics/VideoFrameResourceProvider.h"
#include "public/platform/WebVideoFrameSubmitter.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom-blink.h"

namespace blink {
class PLATFORM_EXPORT VideoFrameSubmitter
    : public WebVideoFrameSubmitter,
      NON_EXPORTED_BASE(public viz::mojom::blink::CompositorFrameSinkClient) {
 public:
  VideoFrameSubmitter(
      cc::VideoFrameProvider*,
      base::Callback<
          void(mojo::Binding<viz::mojom::blink::CompositorFrameSinkClient>*,
               viz::mojom::blink::CompositorFrameSinkPtr*)>);

  ~VideoFrameSubmitter() override;

  static void CreateCompositorFrameSink(
      const viz::FrameSinkId,
      mojo::Binding<viz::mojom::blink::CompositorFrameSinkClient>*,
      viz::mojom::blink::CompositorFrameSinkPtr*);

  // VideoFrameProvider::Client implementation.
  void StopUsingProvider() override;
  void StartRendering() override;
  void StopRendering() override;
  void DidReceiveFrame() override;

  void SubmitFrame(viz::BeginFrameAck);
  bool Rendering() { return rendering_; };
  cc::VideoFrameProvider* Provider() { return provider_; }

  // cc::mojom::CompositorFrameSinkClient implementation.
  void DidReceiveCompositorFrameAck(
      const WTF::Vector<viz::ReturnedResource>& resources) override;
  void OnBeginFrame(const viz::BeginFrameArgs&) override;
  void OnBeginFramePausedChanged(bool paused) override {}
  void ReclaimResources(
      const WTF::Vector<viz::ReturnedResource>& resources) override {}

 private:
  cc::VideoFrameProvider* provider_;
  viz::mojom::blink::CompositorFrameSinkPtr compositor_frame_sink_;
  mojo::Binding<viz::mojom::blink::CompositorFrameSinkClient> binding_;
  viz::LocalSurfaceIdAllocator local_surface_id_allocator_;
  viz::LocalSurfaceId current_local_surface_id_;
  VideoFrameResourceProvider resource_provider_;

  bool rendering_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameSubmitter);
};

}  // namespace blink
