// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasResourceProvider_h
#define CanvasResourceProvider_h

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "cc/paint/skia_paint_canvas.h"
#include "cc/raster/playback_image_provider.h"
#include "platform/geometry/IntSize.h"
#include "platform/graphics/CanvasColorParams.h"
#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/Optional.h"
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/Vector.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/skia/include/core/SkSurface.h"

class GrContext;
class SkCanvas;

namespace cc {
class ImageDecodeCache;
class PaintCanvas;
}

namespace gpu {
namespace gles2 {

class GLES2Interface;

}  // namespace gles2
}  // namespace gpu

namespace blink {

class CanvasResource;
class StaticBitmapImage;
class WebGraphicsContext3DProviderWrapper;

// CanvasResourceProvider
//==============================================================================
//
// This is an abstract base class that encapsulates a drawable graphics
// resource.  Subclasses manage specific resource types (Gpu Textures,
// GpuMemoryBuffer, Bitmap in RAM). CanvasResourceProvider serves as an
// abstraction layer for these resource types. It is designed to serve
// the needs of Canvas2DLayerBridge, but can also be used as a general purpose
// provider of drawable surfaces for 2D rendering with skia.
//
// General usage:
//   1) Use the Create() static method to create an instance
//   2) use Canvas() to get a drawing interface
//   3) Call Snapshot() to acquire a bitmap with the rendered image in it.

class PLATFORM_EXPORT CanvasResourceProvider
    : public WebGraphicsContext3DProviderWrapper::DestructionObserver {
  WTF_MAKE_NONCOPYABLE(CanvasResourceProvider);

 public:
  enum ResourceUsage {
    kSoftwareResourceUsage,
    kSoftwareCompositedResourceUsage,
    kAcceleratedResourceUsage,
    kAcceleratedCompositedResourceUsage,
  };

  static std::unique_ptr<CanvasResourceProvider> Create(
      const IntSize&,
      ResourceUsage,
      base::WeakPtr<WebGraphicsContext3DProviderWrapper> = nullptr,
      unsigned msaa_sample_count = 0,
      const CanvasColorParams& = CanvasColorParams());

  // Use this method for capturing a frame that is intended to be displayed via
  // the compositor. Cases that need to acquire a snaptshot that is not destined
  // to be transfered via TransferableResource should call Snapshot() instead.
  virtual scoped_refptr<CanvasResource> ProduceFrame() = 0;
  scoped_refptr<StaticBitmapImage> Snapshot();

  // WebGraphicsContext3DProvider::DestructionObserver implementation.
  void OnContextDestroyed() override;

  cc::PaintCanvas* Canvas();
  void ReleaseLockedImages();
  void FlushSkia() const;
  const CanvasColorParams& ColorParams() const { return color_params_; }
  void SetFilterQuality(SkFilterQuality quality) { filter_quality_ = quality; }
  const IntSize& Size() const { return size_; }
  virtual bool IsValid() const = 0;
  virtual bool IsAccelerated() const = 0;
  uint32_t ContentUniqueID() const;
  void ClearRecycledResources();
  void RecycleResource(scoped_refptr<CanvasResource>);
  void SetResourceRecyclingEnabled(bool);
  SkSurface* GetSkSurface() const;
  bool IsGpuContextLost() const;
  bool WritePixels(const SkImageInfo& orig_info,
                   const void* pixels,
                   size_t row_bytes,
                   int x,
                   int y);
  virtual GLuint GetBackingTextureHandleForOverwrite() {
    NOTREACHED();
    return 0;
  }
  void Clear();
  ~CanvasResourceProvider() override;

 protected:
  gpu::gles2::GLES2Interface* ContextGL() const;
  GrContext* GetGrContext() const;
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> ContextProviderWrapper() {
    return context_provider_wrapper_;
  }
  scoped_refptr<CanvasResource> NewOrRecycledResource();
  SkFilterQuality FilterQuality() const { return filter_quality_; }
  base::WeakPtr<CanvasResourceProvider> CreateWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  // Called by subclasses when the backing resource has changed and resources
  // are not managed by skia, signaling that a new surface needs to be created.
  void InvalidateSurface();

  CanvasResourceProvider(const IntSize&,
                         const CanvasColorParams&,
                         base::WeakPtr<WebGraphicsContext3DProviderWrapper>);

 private:
  class CanvasImageProvider : public cc::ImageProvider {
   public:
    CanvasImageProvider(cc::ImageDecodeCache*,
                        const gfx::ColorSpace& target_color_space);
    ~CanvasImageProvider() override;

    // cc::ImageProvider implementation.
    ScopedDecodedDrawImage GetDecodedDrawImage(const cc::DrawImage&) override;

    void ReleaseLockedImages();

   private:
    void CanUnlockImage(ScopedDecodedDrawImage);

    std::vector<ScopedDecodedDrawImage> locked_images_;
    cc::PlaybackImageProvider playback_image_provider_;
  };

  virtual sk_sp<SkSurface> CreateSkSurface() const = 0;
  virtual scoped_refptr<CanvasResource> CreateResource();
  cc::ImageDecodeCache* ImageDecodeCache();

  base::WeakPtrFactory<CanvasResourceProvider> weak_ptr_factory_;
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  IntSize size_;
  CanvasColorParams color_params_;
  Optional<CanvasImageProvider> canvas_image_provider_;
  std::unique_ptr<cc::SkiaPaintCanvas> canvas_;
  mutable sk_sp<SkSurface> surface_;  // mutable for lazy init
  std::unique_ptr<SkCanvas> xform_canvas_;
  WTF::Vector<scoped_refptr<CanvasResource>> recycled_resources_;
  SkFilterQuality filter_quality_;
  bool resource_recycling_enabled_ = true;
};

}  // namespace blink

#endif
