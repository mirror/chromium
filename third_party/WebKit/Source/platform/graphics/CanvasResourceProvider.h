// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasResourceProvider_h
#define CanvasResourceProvider_h

#include "platform/geometry/IntSize.h"
#include "platform/graphics/CanvasColorParams.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/WeakPtr.h"
#include "third_party/khronos/GLES2/gl2.h"

class GrContext;
class SkSurface;

namespace cc {

class PaintCanvas;
}

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

class StaticBitmapImage;
class WebGraphicsContext3DProviderWrapper;

class CanvasResourceProvider : public RefCounted<CanvasResourceProvider> {
  WTF_MAKE_NONCOPYABLE(CanvasResourceProvider);

 public:
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

  cc::PaintCanvas* Canvas();
  const CanvasColorParams& ColorParams() const { return color_params_; }
  RefPtr<StaticBitmapImage> Snapshot();

  virtual void WillOverwriteCanvas() {}
  virtual void WillDraw() {}
  void setFilterQuality(SkFilterQuality quality) { filter_quality_ = quality; }

  bool PrepareTextureMailbox(viz::TextureMailbox*,
                             std::unique_ptr<viz::SingleReleaseCallback>*);
  const IntSize& Size() const { return size_; }

  virtual bool IsValid() const = 0;

  virtual bool CanPrepareTextureMailbox() const = 0;
  virtual uint32_t ContentUniqueID() const = 0;

 protected:
  gpu::gles2::GLES2Interface* ContextGL() const;
  GrContext* GetGrContext() const;
  const gpu::Mailbox& GpuMailbox();
  WeakPtr<WebGraphicsContext3DProviderWrapper> ContextProviderWrapper() {
    return context_provider_wrapper_;
  }
  bool IsGpuContextLost() const;
  SkSurface* GetSkSurface() const;
  GLenum GetGLFilter() const;
  void ResetSkiaTextureBinding() const;
  RefPtr<CanvasResource> NewOrRecycledResource();

  // Called by subclasses when the backing resource has changed and resources
  // are not managed by skia, signaling that a new surface needs to be created.
  void InvalidateSurface();

  CanvasResourceProvider(const IntSize&,
                         const CanvasColorParams&,
                         WeakPtr<WebGraphicsContext3DProviderWrapper>);
  virtual ~CanvasResourceProvider();

 private:
  virtual sk_sp<SkSurface> CreateSurface() = 0;
  virtual RefPtr<StaticBitmapImage> CreateSnapshot() = 0;
  virtual RefPtr<StaticBitmapImage> CreateReadOnlyImage();
  virtual RefPtr<CanvasResource> PrepareTextureMailboxAndSwap(
      viz::TextureMailbox* out_mailbox) = 0;
  void ValidateState();

  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  IntSize size_;
  CanvasColorParams color_params_;
  std::unique_ptr<cc::PaintCanvas> canvas_;
  sk_sp<SkSurface> surface_;
  WTF::Vector<std::unique_ptr<CanvasResource>> recycled_resources_;
  SkFilterQuality filter_quality_;
};

}  // namespace blink

#endif
