// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasResourceProvider.h"

#include "cc/paint/skia_paint_canvas.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "platform/graphics/CanvasResource.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "platform/graphics/gpu/SharedGpuContext.h"
#include "platform/runtime_enabled_features.h"
#include "skia/ext/texture_handle.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace {

bool CanUseGpuMemoryBuffer(
    const WeakPtr<blink::WebGraphicsContext3DProviderWrapper>&
        context_provider_wrapper,
    const blink::CanvasColorParams& colorParams,
    const blink::IntSize& size) {
  if (!blink::RuntimeEnabledFeatures::Canvas2dImageChromiumEnabled())
    return false;
  if (!blink::SharedGpuContext::IsGpuCompositingEnabled())
    return false;
  if (!gpu::IsImageFromGpuMemoryBufferFormatSupported(
          colorParams.GetBufferFormat(),
          context_provider_wrapper->ContextProvider()->GetCapabilities()))
    return false;
  if (!gpu::IsImageSizeValidForGpuMemoryBufferFormat(
          gfx::Size(size), colorParams.GetBufferFormat()))
    return false;
  DCHECK(gpu::IsImageFormatCompatibleWithGpuMemoryBufferFormat(
      colorParams.GLInternalFormat(), colorParams.GetBufferFormat()));
  return true;
}

}  // unnamed namespace

namespace blink {

// CanvasResourceProvider_Texture
//==============================================================================
//
// * Renders to a texture managed by skia. Mailboxes are straight GL textures.
// * Layers are not overlay candidates

class CanvasResourceProvider_Texture : public CanvasResourceProvider {
 public:
  CanvasResourceProvider_Texture(
      const IntSize& size,
      unsigned msaa_sample_count,
      const CanvasColorParams color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider(size,
                               color_params,
                               std::move(context_provider_wrapper)),
        msaa_sample_count_(msaa_sample_count) {}

  virtual ~CanvasResourceProvider_Texture() {}

  bool IsValid() const override {
    return GetSkSurface() && !IsGpuContextLost();
  }
  bool IsAccelerated() const final { return true; }

  bool CanPrepareTextureMailbox() const override { return true; }

 protected:
  static gfx::BufferUsage BufferUsageForLowLatencyMode() {
    return gfx::BufferUsage::SCANOUT;
  }

  scoped_refptr<StaticBitmapImage> CreateSnapshot() override {
    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();
    if (!image)
      return nullptr;
    DCHECK(image->isTextureBacked());
    return StaticBitmapImage::Create(image, ContextProviderWrapper());
  }

  scoped_refptr<CanvasResource> DoPrepareTextureMailbox(
      viz::TextureMailbox* out_mailbox) override {
    DCHECK(GetSkSurface());

    if (IsGpuContextLost())
      return nullptr;

    auto gl = ContextGL();
    DCHECK(gl);

    if (ContextProviderWrapper()
            ->ContextProvider()
            ->GetCapabilities()
            .disable_2d_canvas_copy_on_write) {
      GetSkSurface()->notifyContentWillChange(
          SkSurface::kRetain_ContentChangeMode);
    }

    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();
    if (!image)
      return nullptr;
    DCHECK(image->isTextureBacked());

    scoped_refptr<CanvasResource> resource =
        CanvasResource_Skia::Create(image, ContextProviderWrapper());
    ResourceToMailbox(resource.get(), out_mailbox);
    image->getTexture()->textureParamsModified();

    return resource;
  }

  virtual sk_sp<SkSurface> CreateSkSurface() const {
    if (IsGpuContextLost())
      return nullptr;
    auto gr = GetGrContext();
    DCHECK(gr);

    SkImageInfo info = SkImageInfo::Make(
        Size().Width(), Size().Height(), ColorParams().GetSkColorType(),
        kPremul_SkAlphaType, ColorParams().GetSkColorSpaceForSkSurfaces());
    return SkSurface::MakeRenderTarget(gr, SkBudgeted::kNo, info,
                                       msaa_sample_count_,
                                       ColorParams().GetSkSurfaceProps());
  }

  unsigned msaa_sample_count_;
};

// CanvasResourceProvider_Texture_GpuMemoryBuffer
//==============================================================================
//
// * Renders to a texture managed by skia. Mailboxes are
//     gpu-accelerated platform native surfaces.
// * Layers are overlay candidates

class CanvasResourceProvider_Texture_GpuMemoryBuffer final
    : public CanvasResourceProvider_Texture {
 public:
  CanvasResourceProvider_Texture_GpuMemoryBuffer(
      const IntSize& size,
      unsigned msaa_sample_count,
      const CanvasColorParams color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider_Texture(size,
                                       msaa_sample_count,
                                       color_params,
                                       std::move(context_provider_wrapper)) {}

  bool SupportsRecycling() const override { return true; }

  virtual ~CanvasResourceProvider_Texture_GpuMemoryBuffer() {}

 protected:
  bool IsOverlayCandidate() override { return true; }

  scoped_refptr<CanvasResource> CreateResource() final {
    return CanvasResource_GpuMemoryBuffer::Create(Size(), ColorParams(),
                                                  gfx::BufferUsage::SCANOUT,
                                                  ContextProviderWrapper());
  }

  scoped_refptr<CanvasResource> DoPrepareTextureMailbox(
      viz::TextureMailbox* out_mailbox) final {
    DCHECK(GetSkSurface());

    if (IsGpuContextLost())
      return nullptr;

    scoped_refptr<CanvasResource> output_resource = NewOrRecycledResource();
    if (!output_resource) {
      // GpuMemoryBuffer creation failed, fallback to Texture resource
      return CanvasResourceProvider_Texture::DoPrepareTextureMailbox(
          out_mailbox);
    }

    auto gl = ContextGL();
    auto gr = GetGrContext();
    DCHECK(gl && gr);

    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();
    if (!image)
      return nullptr;
    DCHECK(image->isTextureBacked());

    GLuint skia_texture_id =
        skia::GrBackendObjectToGrGLTextureInfo(image->getTextureHandle(true))
            ->fID;

    gl->CopyTextureCHROMIUM(
        skia_texture_id, 0 /*sourceLevel*/, output_resource->TextureTarget(),
        output_resource->TextureId(), 0 /*destLevel*/,
        ColorParams().GLInternalFormat(), ColorParams().GLType(),
        false /*unpackFlipY*/, false /*unpackPremultiplyAlpha*/,
        false /*unpackUnmultiplyAlpha*/);

    ResourceToMailbox(output_resource.get(), out_mailbox);
    return output_resource;
  }
};

// CanvasResourceProvider_Bitmap
//==============================================================================
//
// * Renders to a skia RAM-backed bitmap
// * Mailboxing is not supported : cannot be directly composited

class CanvasResourceProvider_Bitmap : public CanvasResourceProvider {
 public:
  // Note : context_provider_wrapper is not required when instantiating
  // CanvasResourceProvider_Bitmap directly. It is required by derived class
  // CanvasResourceProvider_LowLatency.
  CanvasResourceProvider_Bitmap(const IntSize& size,
                                const CanvasColorParams color_params,
                                WeakPtr<WebGraphicsContext3DProviderWrapper>
                                    context_provider_wrapper = nullptr)
      : CanvasResourceProvider(size, color_params, context_provider_wrapper) {}

  // For templating purposes, we need a constructor with the same args as
  // CanvasResourceProvider_Texture.  The second arg is simply ignored.
  CanvasResourceProvider_Bitmap(const IntSize& size,
                                unsigned,
                                const CanvasColorParams color_params,
                                WeakPtr<WebGraphicsContext3DProviderWrapper>
                                    context_provider_wrapper = nullptr)
      : CanvasResourceProvider(size, color_params, context_provider_wrapper) {}

  ~CanvasResourceProvider_Bitmap() {}

  bool IsValid() const override { return GetSkSurface(); }
  bool IsAccelerated() const final { return false; }

  bool CanPrepareTextureMailbox() const override { return false; }

 protected:
  static gfx::BufferUsage BufferUsageForLowLatencyMode() {
    return gfx::BufferUsage::GPU_READ_CPU_READ_WRITE_PERSISTENT;
  }

  scoped_refptr<StaticBitmapImage> CreateSnapshot() override {
    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();
    if (!image)
      return nullptr;
    return StaticBitmapImage::Create(image);
  }

  scoped_refptr<CanvasResource> DoPrepareTextureMailbox(
      viz::TextureMailbox* out_mailbox) {
    NOTREACHED();  // Not directly compositable.
    return nullptr;
  }

  sk_sp<SkSurface> CreateSkSurface() const override {
    SkImageInfo info = SkImageInfo::Make(
        Size().Width(), Size().Height(), ColorParams().GetSkColorType(),
        kPremul_SkAlphaType, ColorParams().GetSkColorSpaceForSkSurfaces());
    return SkSurface::MakeRaster(info, ColorParams().GetSkSurfaceProps());
  }
};

// CanvasResourceProvider_LowLatency
//==============================================================================
//
// * Renders to a skia surface, the type of which is determined by BaseClass
// * BaseClass can be CanvasResourceProvider_Bitmap or
//   CanvasResourceProvider_Texture
// * Mailboxing uses a single-buffered GpuMemoryBuffer if GpuMemoryBuffers are
//   enabled, otherwise it use a GL texture
// * Presentation is subject to tearing artifacts and frame order inversion is
//   possible when switching out of hw overlay mode.
// * Template argument can be CanvasResourceProvider_Bitmap or
//   CanvasResourceProvider_Texture.
//

template <class BaseClass>
class CanvasResourceProvider_LowLatency : public BaseClass {
 public:
  CanvasResourceProvider_LowLatency(
      const IntSize& size,
      unsigned msaa_sample_count,
      const CanvasColorParams color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : BaseClass(size,
                  msaa_sample_count,
                  color_params,
                  context_provider_wrapper) {
    // SingleBuffered means output resource is owned here
    if (CanUseGpuMemoryBuffer(context_provider_wrapper, color_params, size)) {
      front_buffer_ = CanvasResource_GpuMemoryBuffer::Create(
          BaseClass::Size(), BaseClass::ColorParams(),
          BaseClass::BufferUsageForLowLatencyMode(),
          BaseClass::ContextProviderWrapper());
    } else if (BaseClass::IsAccelerated()) {
      front_buffer_ = CanvasResource_Texture::Create(
          BaseClass::Size(), BaseClass::ColorParams(),
          BaseClass::ContextProviderWrapper());
    }
  }

  bool IsValid() const final {
    return front_buffer_ && BaseClass::GetSkSurface();
  }
  bool IsFlipped() const final { return false; }
  bool CanPrepareTextureMailbox() const final { return true; }
  bool ReusesMailboxes() const final { return true; }
  void DidDraw(const FloatRect& rect) final { dirty_rect_.Unite(rect); }

 protected:
  bool IsOverlayCandidate() override { return true; }

  scoped_refptr<CanvasResource> DoPrepareTextureMailbox(
      viz::TextureMailbox* out_mailbox) final {
    DCHECK(BaseClass::GetSkSurface());

    if (BaseClass::IsGpuContextLost() || !front_buffer_)
      return nullptr;

    BaseClass::ResourceToMailbox(front_buffer_.get(), out_mailbox);
    // Returning the front buffer will allow the compositor to keep the
    // resource alive after the resource provider has been destroyed.
    return front_buffer_;
  }

  void UpdateFrontBuffer() final {
    sk_sp<SkImage> image = BaseClass::GetSkSurface()->makeImageSnapshot();
    if (!image || dirty_rect_.IsEmpty())
      return;

    IntRect int_dirty_rect = EnclosingIntRect(dirty_rect_);
    int_dirty_rect.Inflate(1);
    int_dirty_rect.Intersect(IntRect(IntPoint(0, 0), BaseClass::Size()));
    front_buffer_->UpdateContents(image.get(), int_dirty_rect);
    dirty_rect_ = FloatRect();

    // This flush helps lower latency.  Without it, the compositor may only
    // pull gl commands from the stream up to the SyncPoint of the frame
    // currently being presented.  This flush pushes the stream
    // to commit commands beyond the SyncPoints of previous frames, thus
    // allowing content to immediately catch up to frames that have already
    // been committed.
    auto gl = BaseClass::ContextGL();
    if (gl)
      gl->Flush();
  }

 private:
  scoped_refptr<CanvasResource> front_buffer_;
  FloatRect dirty_rect_;
};

// CanvasResourceProvider base class implementation
//==============================================================================

enum ResourceType {
  kTextureGpuMemoryBufferResourceType,
  kTextureResourceType,
  kBitmapResourceType,
  kLowLatencyBitmapResourceType,
  kLowLatencyTextureResourceType,
};

ResourceType kSoftwareCompositedFallbackList[] = {
    kBitmapResourceType,
};

ResourceType kSoftwareFallbackList[] = {
    kBitmapResourceType,
};

ResourceType kAcceleratedFallbackList[] = {
    kTextureResourceType, kBitmapResourceType,
};

ResourceType kAcceleratedCompositedFallbackList[] = {
    kTextureGpuMemoryBufferResourceType, kTextureResourceType,
    kBitmapResourceType,
};

ResourceType kLowLatencyFallbackList[] = {
#if defined(OS_CHROMEOS)
    kLowLatencyBitmapResourceType,
#endif
    kLowLatencyTextureResourceType, kTextureGpuMemoryBufferResourceType,
    kTextureResourceType, kBitmapResourceType};

std::unique_ptr<CanvasResourceProvider> CanvasResourceProvider::Create(
    const IntSize& size,
    unsigned msaa_sample_count,
    const CanvasColorParams& colorParams,
    ResourceUsage usage,
    WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
  unsigned resourceTypeFallbackListLength = 0;
  ResourceType* resourceTypeFallbackList = nullptr;

  switch (usage) {
#define DEFINE_FALLBACK_LIST(NAME)                         \
  case NAME##ResourceUsage:                                \
    resourceTypeFallbackList = NAME##FallbackList;         \
    resourceTypeFallbackListLength =                       \
        sizeof(NAME##FallbackList) / sizeof(ResourceType); \
    break;

    DEFINE_FALLBACK_LIST(kSoftware);
    DEFINE_FALLBACK_LIST(kSoftwareComposited);
    DEFINE_FALLBACK_LIST(kAccelerated);
    DEFINE_FALLBACK_LIST(kAcceleratedComposited);
    DEFINE_FALLBACK_LIST(kLowLatency);
  }

  std::unique_ptr<CanvasResourceProvider> provider;
  for (unsigned i = 0; i < resourceTypeFallbackListLength; i++) {
    switch (resourceTypeFallbackList[i]) {
      case kLowLatencyBitmapResourceType:
        if (!CanUseGpuMemoryBuffer(context_provider_wrapper, colorParams, size))
          continue;
        provider = std::make_unique<
            CanvasResourceProvider_LowLatency<CanvasResourceProvider_Bitmap>>(
            size, msaa_sample_count, colorParams, context_provider_wrapper);
        break;
      case kLowLatencyTextureResourceType:
        provider = std::make_unique<
            CanvasResourceProvider_LowLatency<CanvasResourceProvider_Texture>>(
            size, msaa_sample_count, colorParams, context_provider_wrapper);
        break;
      case kTextureGpuMemoryBufferResourceType:
        if (!CanUseGpuMemoryBuffer(context_provider_wrapper, colorParams, size))
          continue;
        provider =
            std::make_unique<CanvasResourceProvider_Texture_GpuMemoryBuffer>(
                size, msaa_sample_count, colorParams, context_provider_wrapper);
        break;
      case kTextureResourceType:
        DCHECK(SharedGpuContext::IsGpuCompositingEnabled());
        provider = std::make_unique<CanvasResourceProvider_Texture>(
            size, msaa_sample_count, colorParams, context_provider_wrapper);
        break;
      case kBitmapResourceType:
        provider =
            std::make_unique<CanvasResourceProvider_Bitmap>(size, colorParams);
        break;
    }
    if (provider && provider->IsValid())
      return provider;
  }

  return nullptr;
}

CanvasResourceProvider::CanvasResourceProvider(
    const IntSize& size,
    const CanvasColorParams& color_params,
    WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
    : weak_ptr_factory_(this),
      context_provider_wrapper_(std::move(context_provider_wrapper)),
      size_(size),
      color_params_(color_params),
      dirty_rect_(FloatPoint(0, 0), FloatSize(size)) {}

CanvasResourceProvider::~CanvasResourceProvider() {}

SkSurface* CanvasResourceProvider::GetSkSurface() const {
  if (!surface_) {
    surface_ = CreateSkSurface();
  }
  return surface_.get();
}

PaintCanvas* CanvasResourceProvider::Canvas() {
  if (!canvas_) {
    if (ColorParams().NeedsSkColorSpaceXformCanvas()) {
      canvas_ = std::make_unique<cc::SkiaPaintCanvas>(
          GetSkSurface()->getCanvas(), ColorParams().GetSkColorSpace());
    } else {
      canvas_ =
          std::make_unique<cc::SkiaPaintCanvas>(GetSkSurface()->getCanvas());
    }
  }
  return canvas_.get();
}

scoped_refptr<StaticBitmapImage> CanvasResourceProvider::Snapshot() {
  if (!IsValid())
    return nullptr;
  scoped_refptr<StaticBitmapImage> image = StaticBitmapImage::Create(
      GetSkSurface()->makeImageSnapshot(), ContextProviderWrapper());
  if (IsAccelerated()) {
    // A readback operation may alter the texture parameters, which may affect
    // the compositor's behavior. Therefore, we must trigger copy-on-write
    // even though we are not technically writing to the texture, only to its
    // parameters.
    // If this issue with readback affecting stat is ever fixed, then we'll
    // have to do this instead of triggering a copy-on-write:
    // static_cast<AcceleratedStaticBitmapImage*>(image.get())
    //   ->RetainOriginalSkImageForCopyOnWrite();
    GetSkSurface()->notifyContentWillChange(
        SkSurface::kRetain_ContentChangeMode);
  }
  return image;
}

gpu::gles2::GLES2Interface* CanvasResourceProvider::ContextGL() const {
  if (!context_provider_wrapper_)
    return nullptr;
  return context_provider_wrapper_->ContextProvider()->ContextGL();
}

GrContext* CanvasResourceProvider::GetGrContext() const {
  if (!context_provider_wrapper_)
    return nullptr;
  return context_provider_wrapper_->ContextProvider()->GetGrContext();
}

void CanvasResourceProvider::FlushSkia() const {
  GetSkSurface()->flush();
}

static void ReleaseFrameResources(
    WeakPtr<CanvasResourceProvider> resource_provider,
    scoped_refptr<CanvasResource> resource,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  resource->SetSyncTokenForRelease(sync_token);
  if (lost_resource) {
    resource->Abandon();
  }
  if (resource_provider && resource_provider->SupportsRecycling() &&
      !lost_resource) {
    resource_provider->RecycleResource(std::move(resource));
  }
}

void CanvasResourceProvider::RecycleResource(
    scoped_refptr<CanvasResource> resource) {
  if (resource_recycling_enabled_)
    recycled_resources_.push_back(std::move(resource));
}

void CanvasResourceProvider::SetResourceRecyclingEnabled(bool value) {
  resource_recycling_enabled_ = value;
  if (!resource_recycling_enabled_)
    ClearRecycledResources();
}

scoped_refptr<CanvasResource> CanvasResourceProvider::NewOrRecycledResource() {
  if (recycled_resources_.size()) {
    scoped_refptr<CanvasResource> resource =
        std::move(recycled_resources_.back());
    recycled_resources_.pop_back();
    // Recycling implies releasing the old content
    resource->WaitSyncTokenBeforeRelease();
    return resource;
  }
  return CreateResource();
}

bool CanvasResourceProvider::PrepareTextureMailbox(
    viz::TextureMailbox* out_mailbox,
    std::unique_ptr<viz::SingleReleaseCallback>* out_callback) {
  DCHECK(CanPrepareTextureMailbox());
  scoped_refptr<CanvasResource> resource = DoPrepareTextureMailbox(out_mailbox);
  if (!resource)
    return false;
  auto func =
      WTF::Bind(&ReleaseFrameResources, weak_ptr_factory_.CreateWeakPtr(),
                WTF::Passed(std::move(resource)));
  *out_callback = viz::SingleReleaseCallback::Create(
      ConvertToBaseCallback(std::move(func)));
  return true;
}

void CanvasResourceProvider::ResourceToMailbox(
    CanvasResource* resource,
    viz::TextureMailbox* out_mailbox) {
  auto gl = ContextGL();
  DCHECK(gl);

  GLuint texture_id = resource->TextureId();
  GLenum target = resource->TextureTarget();
  gl->BindTexture(target, texture_id);
  gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GetGLFilter());
  gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GetGLFilter());
  gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  auto mailbox = resource->GpuMailbox();
  gl->ProduceTextureDirectCHROMIUM(texture_id, target, mailbox.name);
  const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
  gl->ShallowFlushCHROMIUM();
  gpu::SyncToken sync_token;
  gl->GenUnverifiedSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

  *out_mailbox = viz::TextureMailbox(mailbox, sync_token, target,
                                     gfx::Size(Size()), IsOverlayCandidate());

  gfx::ColorSpace color_space = ColorParams().GetStorageGfxColorSpace();
  out_mailbox->set_color_space(color_space);
  out_mailbox->set_nearest_neighbor(UseNearestNeighbor());

  gl->BindTexture(target, 0);

  // Because we are changing the texture binding without going through skia,
  // we must dirty the context.
  ResetSkiaTextureBinding();
}

bool CanvasResourceProvider::IsGpuContextLost() const {
  auto gl = ContextGL();
  return !gl || gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR;
}

GLenum CanvasResourceProvider::GetGLFilter() const {
  return UseNearestNeighbor() ? GL_NEAREST : GL_LINEAR;
}

bool CanvasResourceProvider::UseNearestNeighbor() const {
  return filter_quality_ == kNone_SkFilterQuality;
}

void CanvasResourceProvider::ResetSkiaTextureBinding() const {
  GrContext* gr = GetGrContext();
  if (gr)
    gr->resetContext(kTextureBinding_GrGLBackendState);
}

void CanvasResourceProvider::ClearRecycledResources() {
  recycled_resources_.clear();
}

void CanvasResourceProvider::InvalidateSurface() {
  canvas_ = nullptr;
  xform_canvas_ = nullptr;
  surface_ = nullptr;
}

uint32_t CanvasResourceProvider::ContentUniqueID() const {
  return GetSkSurface()->generationID();
}

scoped_refptr<CanvasResource> CanvasResourceProvider::CreateResource() {
  // Needs to be implemented in subclasses that use resource recycling.
  NOTREACHED();
  return nullptr;
}

}  // namespace blink
