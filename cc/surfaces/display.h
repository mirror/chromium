// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SURFACES_DISPLAY_H_
#define CC_SURFACES_DISPLAY_H_

#include "base/memory/scoped_ptr.h"
#include "cc/output/output_surface_client.h"
#include "cc/output/renderer.h"
#include "cc/resources/returned_resource.h"
#include "cc/surfaces/surface_aggregator.h"
#include "cc/surfaces/surface_id.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/surfaces/surfaces_export.h"

namespace gfx {
class Size;
}

namespace cc {

class BlockingTaskRunner;
class DirectRenderer;
class DisplayClient;
class OutputSurface;
class ResourceProvider;
class SharedBitmapManager;
class Surface;
class SurfaceAggregator;
class SurfaceIdAllocator;
class SurfaceFactory;
class TextureMailboxDeleter;

// A Display produces a surface that can be used to draw to a physical display
// (OutputSurface). The client is responsible for creating and sizing the
// surface IDs used to draw into the display and deciding when to draw.
class CC_SURFACES_EXPORT Display : public OutputSurfaceClient,
                                   public RendererClient,
                                   public SurfaceDamageObserver {
 public:
  Display(DisplayClient* client,
          SurfaceManager* manager,
          SharedBitmapManager* bitmap_manager);
  virtual ~Display();

  bool Initialize(scoped_ptr<OutputSurface> output_surface);
  void Resize(SurfaceId id, const gfx::Size& new_size);
  bool Draw();

  SurfaceId CurrentSurfaceId();
  int GetMaxFramesPending();

  // OutputSurfaceClient implementation.
  virtual void DeferredInitialize() override {}
  virtual void ReleaseGL() override {}
  virtual void CommitVSyncParameters(base::TimeTicks timebase,
                                     base::TimeDelta interval) override;
  virtual void SetNeedsRedrawRect(const gfx::Rect& damage_rect) override {}
  virtual void BeginFrame(const BeginFrameArgs& args) override {}
  virtual void DidSwapBuffers() override;
  virtual void DidSwapBuffersComplete() override;
  virtual void ReclaimResources(const CompositorFrameAck* ack) override {}
  virtual void DidLoseOutputSurface() override;
  virtual void SetExternalDrawConstraints(
      const gfx::Transform& transform,
      const gfx::Rect& viewport,
      const gfx::Rect& clip,
      const gfx::Rect& viewport_rect_for_tile_priority,
      const gfx::Transform& transform_for_tile_priority,
      bool resourceless_software_draw) override {}
  virtual void SetMemoryPolicy(const ManagedMemoryPolicy& policy) override;
  virtual void SetTreeActivationCallback(
      const base::Closure& callback) override {}

  // RendererClient implementation.
  virtual void SetFullRootLayerDamage() override {}

  // SurfaceDamageObserver implementation.
  virtual void OnSurfaceDamaged(SurfaceId surface) override;

 private:
  void InitializeRenderer();

  DisplayClient* client_;
  SurfaceManager* manager_;
  SharedBitmapManager* bitmap_manager_;
  SurfaceId current_surface_id_;
  gfx::Size current_surface_size_;
  LayerTreeSettings settings_;
  scoped_ptr<OutputSurface> output_surface_;
  scoped_ptr<ResourceProvider> resource_provider_;
  scoped_ptr<SurfaceAggregator> aggregator_;
  scoped_ptr<DirectRenderer> renderer_;
  scoped_ptr<BlockingTaskRunner> blocking_main_thread_task_runner_;
  scoped_ptr<TextureMailboxDeleter> texture_mailbox_deleter_;

  DISALLOW_COPY_AND_ASSIGN(Display);
};

}  // namespace cc

#endif  // CC_SURFACES_DISPLAY_H_
