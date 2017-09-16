// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasResourceProvider_h
#define CanvasResourceProvider_h

#include "platform/geometry/IntSize.h"
#include "platform/graphics/CanvasColorParams.h"
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/WeakPtr.h"
#include "third_party/khronos/GLES2/gl2.h"

class GrContext;
class SkSurface;

namespace gpu {

struct Mailbox;

namespace gles2 {

class GLES2Interface;

}  // namespace gles2
}  // namespace gpu

namespace viz {

class SingleReleaseCallback;
class TextureMailbox;

}  // namespace viz

namespace blink {

class CanvasResource;
class PaintCanvas;
class StaticBitmapImage;
class WebGraphicsContext3DProviderWrapper;

class CanvasResourceProvider : public RefCounted<CanvasResourceProvider> {
  enum ResourceUsage {
    kSoftwareResourceUsage,
    kSoftwareCompositedResourceUsage,
    kAcceleratedResourceUsage,
    kAcceleratedCompositedResourceUsage,
  };

  static RefPtr<CanvasResourceProvider> Create(
      const IntSize&,
      const CanvasColorParams&,
      ResourceUsage,
      WeakPtr<WebGraphicsContext3DProviderWrapper> = nullptr);

  PaintCanvas* Canvas();
  const CanvasColorParams& ColorParams() const { return color_params_; }
  RefPtr<StaticBitmapImage> Snapshot();
  // ReadOnlyImage is similar to Snapshot, except it never makes a copy. The
  // returned image must be used immediately and never retained.
  RefPtr<StaticBitmapImage> EphemeralSnapshot();
  void WillClearContents();
  void setFilterQuality(SkFilterQuality quality) { filter_quality_ = quality; }

  bool PrepareTextureMailbox(viz::TextureMailbox*,
                             std::unique_ptr<viz::SingleReleaseCallback>*);

  virtual bool IsValid() const = 0;

  virtual bool IsSnapshotExpensive() const = 0;
  virtual bool CanPrepareTextureMailbox() const = 0;
  virtual uint32_t ContentUniqueID() const = 0;

 protected:
  gpu::gles2::GLES2Interface* ContextGL() const;
  GrContext* GetGrContext() const;
  const gpu::Mailbox& GpuMailbox();
  WeakPtr<WebGraphicsContext3DProviderWrapper> ContextProviderWrapper() {
    return context_provider_wrapper_;
  }
  bool IsGpuContextValid() const;
  SkSurface* GetSkSurface();
  GLenum GetGLFilter() const;

  CanvasResourceProvider();
  virtual ~CanvasResourceProvider();

 private:
  virtual sk_sp<SkSurface> CreateSurface() = 0;
  virtual RefPtr<StaticBitmapImage> CreateSnapshot() = 0;
  virtual RefPtr<StaticBitmapImage> CreateReadOnlyImage();
  virtual std::unique_ptr<CanvasResource> PrepareTextureMailboxAndSwap(
      viz::TextureMailbox* out_mailbox,
      std::unique_ptr<CanvasResource> recyled_resource) const = 0;

  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  IntSize size_;
  CanvasColorParams color_params_;
  std::unique_ptr<PaintCanvas> canvas_;
  sk_sp<SkSurface> surface_;
  WTF::Vector<std::unique_ptr<CanvasResource>> recycled_resources_;
  SkFilterQuality filter_quality_;
#if DCHECK_IS_ON()
  // For validating that the return value of ReadOnlyPixels was not retained
  WeakPtr<StaticBitmapImage> read_only_pixels_;
#endif
};

}  // namespace blink

#endif
