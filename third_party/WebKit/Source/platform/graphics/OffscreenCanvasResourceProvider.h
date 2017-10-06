// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OffscreenCanvasResourceProvider_h
#define OffscreenCanvasResourceProvider_h

#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/resources/returned_resource.h"
#include "components/viz/common/resources/transferable_resource.h"
#include "platform/graphics/StaticBitmapImage.h"

namespace blink {

class WebGraphicsContext3DProviderWrapper;

class PLATFORM_EXPORT OffscreenCanvasResourceProvider {
 public:
  OffscreenCanvasResourceProvider(int width, int height);

  ~OffscreenCanvasResourceProvider();

  // Returns a context for the current frame if gpu compositing is being used,
  // and null otherwise. May only be used for a single frame at a time, in order
  // to know the current gpu compositing state. Because this is on another
  // thread, it may return null one frame but non-null the next if it recovers
  // and the compositor hasn't changed modes.
  WeakPtr<WebGraphicsContext3DProviderWrapper>
  GetContextForGpuCompositedFrame();

  void TransferResource(viz::TransferableResource*);
  void SetTransferableResourceToSharedBitmap(viz::TransferableResource&,
                                             RefPtr<StaticBitmapImage>);
  void SetTransferableResourceToSharedGPUContext(
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper,
      viz::TransferableResource&,
      RefPtr<StaticBitmapImage>);
  void SetTransferableResourceToStaticBitmapImage(viz::TransferableResource&,
                                                  RefPtr<StaticBitmapImage>);
  void ReclaimResource(unsigned resource_id);
  void ReclaimResources(const WTF::Vector<viz::ReturnedResource>& resources);
  void IncNextResourceId() { next_resource_id_++; }
  unsigned GetNextResourceId() { return next_resource_id_; }

  void Reshape(int width, int height) {
    width_ = width;
    height_ = height;
  }

 private:
  int width_;
  int height_;
  unsigned next_resource_id_ = 1;

  struct FrameResource {
    RefPtr<StaticBitmapImage> image_;
    std::unique_ptr<viz::SharedBitmap> shared_bitmap_;

    // context_provider_wrapper_ is associated with texture_id_ and image_id.
    WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
    GLuint texture_id_ = 0;
    GLuint image_id_ = 0;

    bool spare_lock_ = true;
    gpu::Mailbox mailbox_;

    FrameResource() {}
    ~FrameResource();
  };

  std::unique_ptr<FrameResource> recycleable_resource_;
  std::unique_ptr<FrameResource> CreateOrRecycleFrameResource();

  void SetNeedsBeginFrameInternal();

  bool using_software_compositing_ = false;

  typedef HashMap<unsigned, std::unique_ptr<FrameResource>> ResourceMap;
  void ReclaimResourceInternal(const ResourceMap::iterator&);
  ResourceMap resources_;
};

}  // namespace blink

#endif  // OffscreenCanvasResourceProvider_h
