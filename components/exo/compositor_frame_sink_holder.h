// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_COMPOSITOR_FRAME_SINK_HOLDER_H_
#define COMPONENTS_EXO_COMPOSITOR_FRAME_SINK_HOLDER_H_

#include <list>
#include <map>
#include <memory>

#include "cc/ipc/mojo_compositor_frame_sink.mojom.h"
#include "cc/resources/single_release_callback.h"
#include "cc/resources/transferable_resource.h"
#include "cc/scheduler/begin_frame_source.h"
#include "components/exo/compositor_frame_sink.h"
#include "components/exo/surface_observer.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace exo {
class Surface;

// This class talks to MojoCompositorFrameSink and keeps track of references to
// the contents of Buffers. It's keeped alive by references from
// release_callbacks_. It's destroyed when its owning Surface is destroyed and
// the last outstanding release callback is called.
class CompositorFrameSinkHolder
    : public base::RefCounted<CompositorFrameSinkHolder>,
      public cc::ExternalBeginFrameSourceClient,
      public cc::mojom::MojoCompositorFrameSinkClient,
      public cc::BeginFrameObserver,
      public SurfaceObserver {
 public:
  CompositorFrameSinkHolder(
      Surface* surface,
      std::unique_ptr<CompositorFrameSink> frame_sink,
      cc::mojom::MojoCompositorFrameSinkClientRequest request);

  bool HasReleaseCallbackForResource(cc::ResourceId id);
  void AddResourceReleaseCallback(
      cc::ResourceId id,
      std::unique_ptr<cc::SingleReleaseCallback> callback);

  CompositorFrameSink* GetCompositorFrameSink() { return frame_sink_.get(); }

  base::WeakPtr<CompositorFrameSinkHolder> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  using FrameCallback = base::Callback<void(base::TimeTicks frame_time)>;
  void ActivateFrameCallbacks(std::list<FrameCallback>* frame_callbacks);
  void CancelFrameCallbacks();

  void SetNeedsBeginFrame(bool needs_begin_frame);

  void Satisfy(const cc::SurfaceSequence& sequence);
  void Require(const cc::SurfaceId& id, const cc::SurfaceSequence& sequence);

  // Overridden from cc::mojom::MojoCompositorFrameSinkClient:
  void DidReceiveCompositorFrameAck() override;
  void OnBeginFrame(const cc::BeginFrameArgs& args) override;
  void ReclaimResources(const cc::ReturnedResourceArray& resources) override;
  void WillDrawSurface() override;

  // Overridden from cc::BeginFrameObserver:
  const cc::BeginFrameArgs& LastUsedBeginFrameArgs() const override;
  void OnBeginFrameSourcePausedChanged(bool paused) override;

  // Overridden from cc::ExternalBeginFrameSouceClient:
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  // Overridden from SurfaceObserver:
  void OnSurfaceDestroying(Surface* surface) override;

 private:
  friend class base::RefCounted<CompositorFrameSinkHolder>;

  ~CompositorFrameSinkHolder() override;

  void UpdateNeedsBeginFrame();

  // Each release callback holds a reference to the CompositorFrameSinkHolder
  // itself to keep it alive. Running and erasing the callbacks might result in
  // the instance being destroyed. Therefore, we should not access any member
  // variables after running and erasing the callbacks.
  using ResourceReleaseCallbackMap =
      std::map<int,
               std::pair<scoped_refptr<CompositorFrameSinkHolder>,
                         std::unique_ptr<cc::SingleReleaseCallback>>>;
  ResourceReleaseCallbackMap release_callbacks_;

  Surface* surface_;
  std::unique_ptr<CompositorFrameSink> frame_sink_;

  std::list<FrameCallback> active_frame_callbacks_;
  std::unique_ptr<cc::ExternalBeginFrameSource> begin_frame_source_;
  bool needs_begin_frame_ = false;
  cc::BeginFrameArgs last_begin_frame_args_;
  mojo::Binding<cc::mojom::MojoCompositorFrameSinkClient> binding_;

  base::WeakPtrFactory<CompositorFrameSinkHolder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CompositorFrameSinkHolder);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_COMPOSITOR_FRAME_SINK_HOLDER_H_
