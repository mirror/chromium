// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasResourceProvider.h"

#include "cc/paint/skia_paint_canvas.h"
#include "components/viz/common/quads/texture_mailbox.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"
#include "platform/runtime_enabled_features.h"
#include "public/platform/Platform.h"
#include "skia/ext/texture_handle.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace {

enum ResourceType {
  kTextureGpuMemoryBufferResourceType,
  kTextureResourceType,
  kBitmapResourceType,
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

}  // unnamed namespace

namespace blink {

using std::unique_ptr;

// Generic resource interface, used for locking (RAII) and recycling pixel
// buffers of any type.
class CanvasResource {
 public:
  virtual ~CanvasResource() { WaitReleaseSyncToken(); }
  virtual void Abandon() = 0;
  virtual bool IsRecycleable() const = 0;
  virtual bool IsValid() const = 0;
  virtual GLuint TextureId() const = 0;

  gpu::gles2::GLES2Interface* ContextGL() const {
    if (!context_provider_wrapper_)
      return nullptr;
    return context_provider_wrapper_->ContextProvider()->ContextGL();
  }

  const gpu::Mailbox& GpuMailbox() {
    if (gpu_mailbox_.IsZero()) {
      auto gl = ContextGL();
      DCHECK(gl);  // caller should already have early exited if !gl.
      if (gl) {
        gl->GenMailboxCHROMIUM(gpu_mailbox_.name);
      }
    }
    return gpu_mailbox_;
  }

  void SetReleaseSyncToken(const gpu::SyncToken& token) {
    release_sync_token_ = token;
  }

  void WaitReleaseSyncToken() {
    auto gl = ContextGL();
    if (release_sync_token_.HasData() && gl) {
      gl->WaitSyncTokenCHROMIUM(release_sync_token_.GetData());
    }
    release_sync_token_.Clear();
  }

 protected:
  CanvasResource(
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : context_provider_wrapper_(std::move(context_provider_wrapper)) {}

  gpu::Mailbox gpu_mailbox_;
  // Sync token that was provided when resource was released
  gpu::SyncToken release_sync_token_;
  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
};

// Resource type for Bitmaps
class CanvasResource_Skia : public CanvasResource {
 public:
  static std::unique_ptr<CanvasResource_Skia> Create(
      sk_sp<SkImage> image,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
    return WTF::WrapUnique(new CanvasResource_Skia(
        std::move(image), std::move(context_provider_wrapper)));
  }

  virtual ~CanvasResource_Skia() {}

  // Not recyclable: Skia handles texture recycling internally and bitmaps are
  // cheap to allocate.
  bool IsRecycleable() const final { return false; }

  bool IsValid() const final {
    if (!image_)
      return false;
    if (!image_->isTextureBacked())
      return true;
    return !!context_provider_wrapper_;
  }

  void Abandon() final {
    WaitReleaseSyncToken();
    image_ = nullptr;
    context_provider_wrapper_ = nullptr;
  }

  GLuint TextureId() const final {
    DCHECK(image_->isTextureBacked());
    return skia::GrBackendObjectToGrGLTextureInfo(
               image_->getTextureHandle(true))
        ->fID;
  }

 private:
  CanvasResource_Skia(
      sk_sp<SkImage> image,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResource(std::move(context_provider_wrapper)),
        image_(std::move(image)) {}

  sk_sp<SkImage> image_;
};

class CanvasResource_GpuMemoryBuffer : public CanvasResource {
 public:
  static std::unique_ptr<CanvasResource_GpuMemoryBuffer> Create(
      const IntSize& size,
      const CanvasColorParams& color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
    return WTF::WrapUnique(new CanvasResource_GpuMemoryBuffer(
        size, color_params, context_provider_wrapper));
  }

  virtual ~CanvasResource_GpuMemoryBuffer() { Abandon(); }

  bool IsRecycleable() const final { return IsValid(); }

  bool IsValid() const { return context_provider_wrapper_ && image_id_; }

  void Abandon() final {
    WaitReleaseSyncToken();
    if (!context_provider_wrapper_ || !image_id_)
      return;
    auto gl = context_provider_wrapper_->ContextProvider()->ContextGL();
    auto gr = context_provider_wrapper_->ContextProvider()->GetGrContext();
    if (gl && texture_id_) {
      GLenum target = GL_TEXTURE_RECTANGLE_ARB;
      gl->BindTexture(target, texture_id_);
      gl->ReleaseTexImage2DCHROMIUM(target, image_id_);
      gl->DestroyImageCHROMIUM(image_id_);
      gl->DeleteTextures(1, &texture_id_);
      gl->BindTexture(target, 0);
      if (gr) {
        gr->resetContext(kTextureBinding_GrGLBackendState);
      }
    }
    image_id_ = 0;
    texture_id_ = 0;
    gpu_memory_buffer_ = nullptr;
  }

  GLuint TextureId() const final { return texture_id_; }

 private:
  CanvasResource_GpuMemoryBuffer(
      const IntSize& size,
      const CanvasColorParams& color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResource(std::move(context_provider_wrapper)),
        color_params_(color_params) {
    if (!context_provider_wrapper_)
      return;
    auto gl = context_provider_wrapper_->ContextProvider()->ContextGL();
    auto gr = context_provider_wrapper_->ContextProvider()->GetGrContext();
    if (!gl || !gr)
      return;
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager =
        Platform::Current()->GetGpuMemoryBufferManager();
    if (!gpu_memory_buffer_manager)
      return;
    gpu_memory_buffer_ = gpu_memory_buffer_manager->CreateGpuMemoryBuffer(
        gfx::Size(size.Width(), size.Height()),
        color_params_.GetBufferFormat(),  // Use format
        gfx::BufferUsage::SCANOUT, gpu::kNullSurfaceHandle);
    image_id_ = gl->CreateImageCHROMIUM(gpu_memory_buffer_->AsClientBuffer(),
                                        size.Width(), size.Height(),
                                        color_params_.GLInternalFormat());
    if (!image_id_) {
      gpu_memory_buffer_ = nullptr;
      return;
    }

    gpu_memory_buffer_->SetColorSpaceForScanout(
        color_params.GetStorageGfxColorSpace());
    gl->GenTextures(1, &texture_id_);
    const GLenum target = GL_TEXTURE_RECTANGLE_ARB;
    gl->BindTexture(target, texture_id_);
    gl->BindTexImage2DCHROMIUM(target, image_id_);
    gr->resetContext(kTextureBinding_GrGLBackendState);
  }

  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
  GLuint image_id_ = 0;
  GLuint texture_id_ = 0;
  CanvasColorParams color_params_;
};

// CanvasResourceProvider_Texture
//==============================================================================

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

  bool IsValid() const final { return GetSkSurface() && !IsGpuContextLost(); }
  bool IsAccelerated() const final { return true; }

  bool CanPrepareTextureMailbox() const final { return true; }

 protected:
  virtual bool isOverlayCandidate() { return false; }

  void ResourceToMailbox(CanvasResource* resource,
                         GLenum target,
                         viz::TextureMailbox* out_mailbox) {
    auto gl = ContextGL();
    auto gr = GetGrContext();
    DCHECK(gl && gr);

    GLuint texture_id = resource->TextureId();

    gl->BindTexture(target, texture_id);
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GetGLFilter());
    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GetGLFilter());
    gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    auto mailbox = resource->GpuMailbox();
    gl->ProduceTextureDirectCHROMIUM(texture_id, GL_TEXTURE_2D, mailbox.name);
    const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
    gl->ShallowFlushCHROMIUM();
    gpu::SyncToken sync_token;
    gl->GenUnverifiedSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

    *out_mailbox = viz::TextureMailbox(mailbox, sync_token, GL_TEXTURE_2D,
                                       gfx::Size(Size()), isOverlayCandidate());

    gfx::ColorSpace color_space = ColorParams().GetStorageGfxColorSpace();
    out_mailbox->set_color_space(color_space);
    out_mailbox->set_nearest_neighbor(UseNearestNeighbor());

    gl->BindTexture(GL_TEXTURE_2D, 0);

    // Because we are changing the texture binding without going through skia,
    // we must dirty the context.
    ResetSkiaTextureBinding();
  }

 private:
  RefPtr<StaticBitmapImage> CreateSnapshot() override {
    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();
    if (!image)
      return nullptr;
    DCHECK(image->isTextureBacked());
    return StaticBitmapImage::Create(image, ContextProviderWrapper());
  }

  std::unique_ptr<CanvasResource> DoPrepareTextureMailbox(
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

    std::unique_ptr<CanvasResource> resource =
        CanvasResource_Skia::Create(image, ContextProviderWrapper());
    ResourceToMailbox(resource.get(), GL_TEXTURE_2D, out_mailbox);
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

class CanvasResourceProvider_Texture_GpuMemoryBuffer
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

  virtual ~CanvasResourceProvider_Texture_GpuMemoryBuffer() {}

 protected:
  bool isOverlayCandidate() override { return true; }

 private:
  std::unique_ptr<CanvasResource> CreateResource() final {
    return CanvasResource_GpuMemoryBuffer::Create(Size(), ColorParams(),
                                                  ContextProviderWrapper());
  }

  std::unique_ptr<CanvasResource> DoPrepareTextureMailbox(
      viz::TextureMailbox* out_mailbox) final {
    DCHECK(GetSkSurface());

    if (IsGpuContextLost())
      return nullptr;

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

    std::unique_ptr<CanvasResource> output_resource = NewOrRecycledResource();
    GLenum target = GL_TEXTURE_RECTANGLE_ARB;

    gl->CopyTextureCHROMIUM(skia_texture_id, 0 /*sourceLevel*/, target,
                            output_resource->TextureId(), 0 /*destLevel*/,
                            ColorParams().GLInternalFormat(),
                            ColorParams().GLType(), false /*unpackFlipY*/,
                            false /*unpackPremultiplyAlpha*/,
                            false /*unpackUnmultiplyAlpha*/);

    ResourceToMailbox(output_resource.get(), target, out_mailbox);
    return output_resource;
  }
};

// CanvasResourceProvider_Bitmap
//==============================================================================

class CanvasResourceProvider_Bitmap : public CanvasResourceProvider {
 public:
  CanvasResourceProvider_Bitmap(const IntSize& size,
                                const CanvasColorParams color_params)
      : CanvasResourceProvider(size,
                               color_params,
                               nullptr /*context_provider_wrapper*/) {}

  ~CanvasResourceProvider_Bitmap() {}

  bool IsValid() const final { return GetSkSurface(); }
  bool IsAccelerated() const final { return false; }

  bool CanPrepareTextureMailbox() const final { return false; }

 private:
  RefPtr<StaticBitmapImage> CreateSnapshot() override {
    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();
    if (!image)
      return nullptr;
    return StaticBitmapImage::Create(image);
  }

  std::unique_ptr<CanvasResource> DoPrepareTextureMailbox(
      viz::TextureMailbox* out_mailbox) final {
    NOTREACHED();  // Not directly compositable.
    return nullptr;
  }

  sk_sp<SkSurface> CreateSkSurface() const override {
    SkImageInfo info = SkImageInfo::Make(
        Size().Width(), Size().Height(), ColorParams().GetSkColorType(),
        kPremul_SkAlphaType, ColorParams().GetSkColorSpaceForSkSurfaces());
    return SkSurface::MakeRaster(info, ColorParams().GetSkSurfaceProps());
  }

  sk_sp<SkSurface> surface_;
};

// CanvasResourceProvider base class implementation
//==============================================================================

std::unique_ptr<CanvasResourceProvider> CanvasResourceProvider::Create(
    const IntSize& size,
    unsigned msaa_sample_count,
    const CanvasColorParams& colorParams,
    ResourceUsage usage,
    WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
  unsigned resourceTypeFallbackListLength = 0;
  ResourceType* resourceTypeFallbackList;

  switch (usage) {
#define DEFINE_FALLBACK(NAME)                              \
  case NAME##ResourceUsage:                                \
    resourceTypeFallbackList = NAME##FallbackList;         \
    resourceTypeFallbackListLength =                       \
        sizeof(NAME##FallbackList) / sizeof(ResourceType); \
    break;

    DEFINE_FALLBACK(kSoftware);
    DEFINE_FALLBACK(kSoftwareComposited);
    DEFINE_FALLBACK(kAccelerated);
    DEFINE_FALLBACK(kAcceleratedComposited);
  }

  std::unique_ptr<CanvasResourceProvider> provider;
  for (unsigned i = 0; i < resourceTypeFallbackListLength; i++) {
    switch (resourceTypeFallbackList[i]) {
      case kTextureGpuMemoryBufferResourceType:
        if (RuntimeEnabledFeatures::Canvas2dImageChromiumEnabled()) {
          provider =
              std::make_unique<CanvasResourceProvider_Texture_GpuMemoryBuffer>(
                  size, msaa_sample_count, colorParams,
                  std::move(context_provider_wrapper));
        }
        break;
      case kTextureResourceType:
        provider = std::make_unique<CanvasResourceProvider_Texture>(
            size, msaa_sample_count, colorParams,
            std::move(context_provider_wrapper));
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
      color_params_(color_params) {}

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

RefPtr<StaticBitmapImage> CanvasResourceProvider::Snapshot() {
  if (!IsValid())
    return nullptr;
  RefPtr<StaticBitmapImage> image = StaticBitmapImage::Create(
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
    unique_ptr<CanvasResource> resource,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  resource->SetReleaseSyncToken(sync_token);
  if (lost_resource) {
    resource->Abandon();
  }
  if (resource_provider && !lost_resource && resource->IsRecycleable()) {
    resource_provider->RecycleResource(std::move(resource));
  }
}

void CanvasResourceProvider::RecycleResource(
    std::unique_ptr<CanvasResource> resource) {
  recycled_resources_.push_back(std::move(resource));
}

std::unique_ptr<CanvasResource>
CanvasResourceProvider::NewOrRecycledResource() {
  if (recycled_resources_.size()) {
    std::unique_ptr<CanvasResource> resource =
        std::move(recycled_resources_.back());
    recycled_resources_.pop_back();
    resource->WaitReleaseSyncToken();
    return resource;
  }
  return CreateResource();
}

bool CanvasResourceProvider::PrepareTextureMailbox(
    viz::TextureMailbox* out_mailbox,
    std::unique_ptr<viz::SingleReleaseCallback>* out_callback) {
  DCHECK(CanPrepareTextureMailbox());
  std::unique_ptr<CanvasResource> resource =
      DoPrepareTextureMailbox(out_mailbox);
  if (!resource)
    return false;
  auto func =
      WTF::Bind(&ReleaseFrameResources, weak_ptr_factory_.CreateWeakPtr(),
                WTF::Passed(std::move(resource)));
  *out_callback = viz::SingleReleaseCallback::Create(
      ConvertToBaseCallback(std::move(func)));
  return true;
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

std::unique_ptr<CanvasResource> CanvasResourceProvider::CreateResource() {
  // Needs to be implemented in subclasses that use resource recycling.
  NOTREACHED();
  return nullptr;
}

}  // namespace blink
