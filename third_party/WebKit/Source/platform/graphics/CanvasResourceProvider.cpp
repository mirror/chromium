// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasResourceProvider.h"

#include "components/viz/common/quads/texture_mailbox.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "platform/graphics/StaticBitmapImage.h"
#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"
#include "public/platform/Platform.h"
#include "skia/ext/texture_handle.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace {

enum ResourceType {
  kGpuMemoryBufferResourceType,
  kTextureResourceType,
  kSharedBitmapResourceType,
  kBitmapResourceType,
};

ResourceType kSoftwareCompositedFallbackList[] = {
    kSharedBitmapResourceType, kBitmapResourceType,
};

ResourceType kSoftwareFallbackList[] = {
    kBitmapResourceType,
};

ResourceType kAcceleratedFallbackList[] = {
    kTextureResourceType, kBitmapResourceType,
};

ResourceType kAcceleratedCompositedFallbackList[] = {
    kGpuMemoryBufferResourceType, kTextureResourceType,
    kSharedBitmapResourceType, kBitmapResourceType,
};

}  // unnamed namespace

namespace blink {

using std::unique_ptr;

// Generic resource interface, used for locking (RAII) and recycling pixel
// buffers of any type.
class CanvasResource : public RefCounted<CanvasResource> {
 public:
  virtual ~CanvasResource();
  virtual void Abandon() {}
  virtual bool IsRecycleable() const = 0;
  virtual bool IsValid() const = 0;
  virtual GLuint TextureId() const { return 0; }
  virtual void CopyContentsFrom(CanvasResource*);

 protected:
  CanvasResource() {}
};

// Resource type for Bitmaps
class CanvasResource_Skia : public CanvasResource {
 public:
  static RefPtr<CanvasResource_Skia> Create(
      sk_sp<SkImage> image,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
    return AdoptRef(new CanvasResource_Skia(image, context_provider_wrapper));
  }

  ~CanvasResource_Skia() final {}

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

  void Abandon() final { NOTREACHED(); }

  void CopyContentsFrom(CanvasResource*) final { NOTREACHED(); }

 private:
  CanvasResource_Skia(
      sk_sp<SkImage> image,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : image_(std::move(image)),
        context_provider_wrapper_(context_provider_wrapper) {}

  sk_sp<SkImage> image_;
  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
};

class CanvasResource_GpuMemoryBuffer : public CanvasResource {
 public:
  static RefPtr<CanvasResource_GpuMemoryBuffer> Create(
      const IntSize& size,
      const CanvasColorParams& color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
    return AdoptRef(new CanvasResource_GpuMemoryBuffer(
        size, color_params, context_provider_wrapper));
  }

  ~CanvasResource_GpuMemoryBuffer() final {
    if (!context_provider_wrapper_ || !image_id_)
      return;
    auto gl = context_provider_wrapper_->ContextProvider()->ContextGL();
    auto gr = context_provider_wrapper_->ContextProvider()->GetGrContext();
    if (gl && gr && texture_id_) {
      GLenum target = GL_TEXTURE_RECTANGLE_ARB;
      gl->BindTexture(target, texture_id_);
      gl->ReleaseTexImage2DCHROMIUM(target, image_id_);
      gl->DestroyImageCHROMIUM(image_id_);
      gl->DeleteTextures(1, &texture_id_);
      gl->BindTexture(target, 0);
      gr->resetContext(kTextureBinding_GrGLBackendState);
    }
  }

  bool IsRecycleable() const final { return IsValid(); }

  bool IsValid() const { return context_provider_wrapper_ && image_id_; }

  void Abandon() final {
    image_id_ = 0;
    texture_id_ = 0;
    gpu_memory_buffer_ = nullptr;
  }

  GLuint TextureId() const final { return texture_id_; }

  void CopyContentsFrom(CanvasResource* other) final {
    if (!IsValid() || !other->IsValid())
      return;
    auto gl = context_provider_wrapper_->ContextProvider()->ContextGL();
    if (!gl)
      return;
    CanvasResource_GpuMemoryBuffer* source =
        static_cast<CanvasResource_GpuMemoryBuffer*>(other);
    DCHECK(context_provider_wrapper_.get() ==
           source->context_provider_wrapper_.get());
    gl->CopyTextureCHROMIUM(
        source->TextureId(), 0 /*sourceLevel*/, GL_TEXTURE_RECTANGLE_ARB,
        TextureId(), 0 /*destLevel*/, color_params_.GLInternalFormat(),
        color_params_.GLType(), false /*unpackFlipY*/,
        false /*unpackPremultiplyAlpha*/, false /*unpackUnmultiplyAlpha*/);
  }

 private:
  CanvasResource_GpuMemoryBuffer(
      const IntSize& size,
      const CanvasColorParams& color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : context_provider_wrapper_(std::move(context_provider_wrapper)),
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
                                        color_params_->GLInternalFormat());
    if (!image_id_) {
      gpu_memory_buffer_ = nullptr;
      return;
    }

    gpu_memory_buffer_->SetColorSpaceForScanout(
        color_params.GetGfxColorSpace());
    gl->GenTextures(1, &texture_id_);
    const GLenum target = GL_TEXTURE_RECTANGLE_ARB;
    gl->BindTexture(target, texture_id_);
    gl->BindTexImage2DCHROMIUM(target, image_id_);
    gr->resetContext(kTextureBinding_GrGLBackendState);
  }

  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
  GLuint image_id_;
  GLuint texture_id_;
  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  CanvasColorParams color_params_;
};
// CanvasResourceProvider_GpuMemoryBuffer
//==============================================================================
//
// * Renders directly to a gpu-accelerated platform native surface
// * Snapshots require making an upfront copy

class CanvasResourceProvider_GpuMemoryBuffer : public CanvasResourceProvider {
 public:
  CanvasResourceProvider_GpuMemoryBuffer(
      const IntSize& size,
      const CanvasColorParams color_params,
      unsigned msaa_sample_count,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider(size,
                               color_params,
                               std::move(context_provider_wrapper)),
        msaa_sample_count_(msaa_sample_count) {
    if (IsGpuContextLost())
      return;
    resource_ = CanvasResource_GpuMemoryBuffer::Create(
        size, color_params, ContextProviderWrapper());
  }

  bool IsValid() const final { return resource_ && resource_->IsValid(); }

  RefPtr<CanvasResource> PrepareTextureMailboxAndSwap(
      viz::TextureMailbox* out_mailbox) final {
    if (!IsValid())
      return nullptr;
    auto gl = ContextGL();
    auto gr = GetGrContext();
    CHECK(gl && gr);  // IsValid() should be a sufficient guarantee

    // Context must be flushed because texture will be accessed outside of skia.
    gr->flush();

    auto mailbox = GpuMailbox();
    const GLenum target = GL_TEXTURE_RECTANGLE_ARB;
    gl->ProduceTextureDirectCHROMIUM(resource_->TextureId(), target,
                                     mailbox.name);
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, GetGLFilter());
    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, GetGLFilter());
    gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
    gl->ShallowFlushCHROMIUM();
    gpu::SyncToken sync_token;
    gl->GenSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

    bool is_overlay_candidate = true;
    bool secure_output_only = false;

    *out_mailbox =
        viz::TextureMailbox(mailbox, sync_token, target, gfx::Size(Size()),
                            is_overlay_candidate, secure_output_only);

    gfx::ColorSpace color_space = ColorParams().GetGfxColorSpace();
    out_mailbox->set_color_space(color_space);

    gl->BindTexture(target, 0);

    // Because we are changing the texture binding without going through skia,
    // we must dirty the context.
    ResetSkiaTextureBinding();

    // This creates an extra reference to the resource, which will trigger a
    // copy on write in WillDraw() or an a new blank resource in
    // WillOverwriteConvas
    return resource_;
  }

  void WillDraw() final {
    if (!resource_->HasOneRef()) {
      // Copy on write
      RefPtr<CanvasResource> old_resource = resource_;
      resource_ = NewOrRecycledResource();
      resource_.CopyContentsFrom(old_resource);
      InvalidateSurface();
    }
  }

  void WillOverwriteCanvas() final {
    if (!resource_.HasOneRef()) {
      // New blank resource
      resource_ = NewOrRecycledResource();
      InvalidateSurface();
    }
  }

 private:
  sk_sp<SkSurface> CreateSurface() final {
    if (!IsValid())
      return nullptr;
    GrGLTextureInfo texture_info;
    texture_info.fTarget = GL_TEXTURE_RECTANGLE_ARB;
    texture_info.fID = resource_->TextureId();
    GrBackendTexture backend_texture(Size().Width(), Size().Height(),
                                     ColorParams().GetGrPixelConfig(),
                                     texture_info);
    return SkSurface::MakeFromBackendTextureAsRenderTarget(
        GetGrContext(), backend_texture, kTopLeft_GrSurfaceOrigin,
        msaa_sample_count_, ColorParams().GetSkColorSpaceForSkSurfaces(),
        ColorParams().GetSkSurfaceProps());
  }

  RefPtr<CanvasResource> CreateResource() {
    return CanvasResource_GpuMemoryBuffer::Create(Size(), ColoreParams(),
                                                  ContextProviderWrapper());
  }

  virtual RefPtr<StaticBitmapImage> CreateSnapshot() {
    DCHECK(GetSkSurface());
    sk_sp<SkImage> image = GetSkSurface()->makeImageSnapshot();

    return StaticBitmapImage::Create(std::move(image), ContextProviderWrapper(),
                                     ResourceHolder(resource_));
  }

  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  RefPtr<CanvasResource> resource_;
  unsigned msaa_sample_count_;
};

// CanvasResourceProvider_Texture
//==============================================================================

class CanvasResourceProvider_Texture : public CanvasResourceProvider {
 public:
  CanvasResourceProvider_Texture(
      const IntSize& size,
      const CanvasColorParams color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider(size,
                               color_params,
                               std::move(context_provider_wrapper)) {}

  ~CanvasResourceProvider_Texture() {}

  bool IsValid() const final { return GetSkSurface() && !IsGpuContextLost(); }

  RefPtr<CanvasResource> PrepareTextureMailboxAndSwap(
      viz::TextureMailbox* out_mailbox) final {
    // Texture resources are recycled by Skia, not CanvasResourceProvider.
    DCHECK(!recycled_resource);
    DCHECK(GetSkSurface());

    if (IsGpuContextLost())
      return nullptr;

    auto gl = ContextGL();
    auto gr = GetGrContext();
    DCHECK(gl && gr);

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

    GLuint texture_id =
        skia::GrBackendObjectToGrGLTextureInfo(image->getTextureHandle(true))
            ->fID;

    gl->BindTexture(GL_TEXTURE_2D, texture_id);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetGLFilter());
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetGLFilter());
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    image->getTexture()->textureParamsModified();

    auto mailbox = GpuMailbox();

    gl->ProduceTextureDirectCHROMIUM(texture_id, GL_TEXTURE_2D, mailbox.name);

    const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
    gl->ShallowFlushCHROMIUM();
    gpu::SyncToken sync_token;
    gl->GenSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

    bool is_overlay_candidate = false;
    bool secure_output_only = false;

    *out_mailbox = viz::TextureMailbox(mailbox, sync_token, GL_TEXTURE_2D,
                                       gfx::Size(Size()), is_overlay_candidate,
                                       secure_output_only);

    gfx::ColorSpace color_space = ColorParams().GetGfxColorSpace();
    out_mailbox->set_color_space(color_space);

    gl->BindTexture(GL_TEXTURE_2D, 0);

    // Because we are changing the texture binding without going through skia,
    // we must dirty the context.
    ResetSkiaTextureBinding();

    return true;
  }

  bool IsResourceManagedBySkia() const final { return true; }
  bool CanPrepareTextureMailbox() const final { return true; }
  uint32_t ContentUniqueID() const final { return surface_->generationID(); }

 private:
  virtual sk_sp<SkSurface> CreateSurface() {
    if (IsGpuContextLost())
      return nullptr;
    auto gr = GetGrContext();
    DCHECK(gr);

    SkImageInfo info = SkImageInfo::Make(
        Size().Width(), Size.Height(), ColorParams().GetSkSurfaceColorType(),
        kPremul_SkAlphaType, ColorParams().GetSkSurfaceColorSpace());
    return SkSurface::MakeRenderTarget(
        gr, SkBudgeted::kNo, info, ColorParams().MSAASampleCount(),
        kTopLeft_GrSurfaceOrigin, ColorParams().GetSkSurfaceProps());
  }

  virtual RefPtr<StaticBitmapImage> CreateSnapshot() {
    if (!IsValid())
      return nullptr;
    return StaticBitmapImage::Create(surface_->makeImageSnapshot());
  }
};

// CanvasResourceProvider_Bitmap
//==============================================================================

class CanvasResourceProvider_Bitmap
    : public RefCounted<CanvasResourceProvider> {
 public:
  CanvasResourceProvider_Bitmap(
      const IntSize& size,
      const CanvasColorParams color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider(size,
                               color_params,
                               std::move(context_provider_wrapper)) {}

  ~CanvasResourceProvider_Bitmap() {}

  bool IsValid() final { return surface_; }

  bool PrepareTextureMailbox(viz::TextureMailbox* out_mailbox) const final {
    NOTREACHED();  // Not directly compositable.
  }

  bool IsResourceManagedBySkia() const final { return true; }
  bool CanPrepareTextureMailbox() const final { return false; }
  uint32_t ContentUniqueID() const final { return surface_->generationID(); }

 private:
  virtual sk_sp<SkSurface> CreateSurface() {
    auto gr = GetGrContext();
    if (!gr)
      return nullptr;
    SkImageInfo info = SkImageInfo::Make(
        Size().Width(), Size.Height(), ColorParams().GetSkSurfaceColorType(),
        kPremul_SkAlphaType, ColorParams().GetSkSurfaceColorSpace());
    return SkSurface::MakeRaster(info, ColorParams().GetSkSurfaceProps());
  }

  virtual RefPtr<StaticBitmapImage> CreateSnapshot() {
    if (!IsValid())
      return nullptr;
    return StaticBitmapImage::Create(surface_->makeImageSnapshot());
  }

  sk_sp<SkSurface> surface_;
};

// CanvasResourceProvider base class implementation
//==============================================================================

RefPtr<CanvasResourceProvider> CanvasResourceProvider::Create(
    const IntSize& size,
    const CanvasColorParams colorParams,
    ResourceUsage usage,
    WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper,
    unsigned msaa_sample_count) {
  unsigned resourceTypeFallbackListLength = 0;
  ResourceType* resourceTypeFallbackList

      switch (usage) {
#define DEFINE_FALLBACK(NAME)                      \
  case NAME##ResourceUsage:                        \
    resourceTypeFallbackList = NAME##FallbackList; \
    resourceTypeFallbackListLength =               \
        sizeof(NAME##FallbackList) / sizeof(ResourceType);

    DEFINE_FALLBACK(kSoftware);
    DEFINE_FALLBACK(kSoftwareComposited);
    DEFINE_FALLBACK(kAccelerated);
    DEFINE_FALLBACK(kAcceleratedComposited);
  }

  RefPtr<CanvasResourceProvider> provider;
  for (unsigned i = 0; i < resourceTypeFallbackListLength; i++) {
    switch (resourceTypeFallbackList[i]) {
      case kGpuMemoryBufferResourceType:
        if (RuntimeEnabledFeatures::Canvas2dImageChromiumEnabled()) {
          provider = new CanvasResourceProvider_GpuMemoryBuffer(
              size, colorParams, std::move(context_provider_wrapper),
              msaa_sample_count);
        }
        break;
      case kTextureResourceType:
        provider = new CanvasResourceProvider_Texture(
            size, colorParams, std::move(context_provider_wrapper),
            msaa_sample_count);
        break;
      case kSharedBitmapResourceType:
        provider = new CanvasResourceProvider_SharedBitmap(
            size, colorParams, std::move(context_provider_wrapper));
        break;
      case kBitmapResourceType:
        provider = new CanvasResourceProvider_Bitmap(size, colorParams);
        break;
    }
    if (provider && provider->IsValid())
      return provider;
  }

  return nullptr;
}

CanvasResourceProvider::CanvasResourceProvider(
    const IntSixe& size,
    const CanvasColorParams& color_params,
    WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
    : context_provider_wrapper_(std::move(context_provider_wrapper)),
      color_params_(color_params) {}

CanvasResourceProvider::~CanvasResourceProvider() {}

PaintCanvas* CanvasResourceProvider::Canvas() {
  if (!canvas_) {
    xform_canvas_ = SkCreateColorSpaceXformCanvas(
        GetSkSurface()->getCanvas(), ColorParams().GetSkColorSpace());
    canvas_ = WTF::MakeUnique<SkiaPaintCanvas>(xform_canvas_);

    if (client_)
      client_->resetCanvasMatrixClipStack(canvas_);
  }

  // ******TODO: wrap in SkColorSpaceXformCanvas?
  return canvas_;
}

ReftPtr<StaticBitmapImage> CanvasResourceProvider::Snapshot() {
  return CreateSnapshot();
}

ReftPtr<StaticBitmapImage> CanvasResourceProvider::EphemeralSnapshot() {
  ReftPtr<StaticBitmapImage> image = CreateEphemeralSnapshot();
#if DCHECK_IS_ON
  outstanding_ephemeral_snapshot_ = image->CreateWeakPtr();
#endif
  return image;
}

virtual ReftPtr<StaticImageBitmap>
CanvasResourceProvider::CreateEphemeralSnapshot() {
  // Overloads for which snapshots are expensive must overload
  // CreateEphemeralSnapshot to provide a lightweight alternative that requires
  // no copying of pixel data.
  DCHECK(!SnapshotIsExpensive());
  return CreateSnapshot();
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

const gpu::Mailbox& CanvasResourceProvider::GpuMailbox() {
  if (gpu_mailbox_.IsZero()) {
    auto gl = ContextGL();
    DCHECK(gl);  // caller should already have early exited if !gl.
    if (gl) {
      gl->GenMailboxCHROMIUM(gpu_mailbox_.name);
    }
  }
  return gpu_mailbox_;
}

static void ReleaseFrameResources(
    WeakPtr<CanvasResourceProvider> resource_provider,
    unique_ptr<CanvasResource> resource,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  resource->WaitSyncToken(sync_token);
  if (lost_resource) {
    resource->Abandon();
  }
  if (resource_provider && !lost_resource && resource->isRecycleable()) {
    resource_provider->RecycleResource(std::move(resource));
  }
}

void CanvasResourceProvider::RecycleResource(RefPtr<CanvasResource> resource) {
  if (resource->HasOneRef()) {
    recycled_resources_.push_back(std::move(resource));
  }
}

RefPtr<CanvasResource> CanvasResourceProvider::NewOrRecycledResource() {
  if (recycled_resources_.Size())
    return recycled_resource.pop_back();
  return CreateResource();
}

bool CanvasResourceProvider::PrepareTextureMailbox(
    viz::TextureMailbox* out_mailbox,
    std::unique_ptr<viz::SingleReleaseCallback>* out_callback) {
  ValidateState();
  DCHECK(CanPrepareTextureMailbox());
  RefPtr<CanvasResource> resource = PrepareTextureMailboxAndSwap(out_mailbox);
  if (!ResourceManagedBySkia()) {
    copy_on_write_source_ = resource;
  }

  auto func =
      WTF::Bind(&ReleaseFrameResources, weak_ptr_factory_.CreateWeakPtr(),
                WTF::Passed(std::move(resource)), out_mailbox->mailbox());
  *out_release_callback = viz::SingleReleaseCallback::Create(
      ConvertToBaseCallback(std::move(func)));
}

bool CanvasResourceProvider::IsGpuContextLost() {
  auto gl = ContextGL();
  return !gl || gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR;
}

GLenum CanvasResourceProvider::GetGLFilter() {
  return filter_quality_ == kNone_SkFilterQuality ? GL_NEAREST : GL_LINEAR;
}

void CanvasResourceProvider::ResetSkiaTextureBinding() const {
  GrContext* gr = GetGrContext();
  if (gr)
    gr->resetContext(kTextureBinding_GrGLBackendState);
}

void CanvasResourcProvider::InvalidateSurface() {
  canvas_ = nullptr;
  xform_canvas_ = nullptr;
  surface_ = nullptr;
}

}  // namespace blink
