// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CanvasResourceProvider.h"

#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"
#include "public/platform/Platform.h"
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

// Resets Skia's texture bindings. This method should be called after
// changing texture bindings.
void ResetSkiaTextureBinding(WeakPtr<blink::WebGraphicsContext3DProviderWrapper>
                                 context_provider_wrapper) {
  if (!context_provider_wrapper)
    return;
  GrContext* gr_context =
      context_provider_wrapper->ContextProvider()->GetGrContext();
  if (gr_context)
    gr_context->resetContext(kTextureBinding_GrGLBackendState);
}

}  // unnamed namespace

namespace blink {

using std::unique_ptr;

// Generic resource interface, used for locking (RAII) and recycling pixel
// buffers of any type.
class CanvasResource : public RefCounted<CanvasResource> {
 public:
  virtual ~CanvasResource();
  virtual void Abandon() {}
  virtual bool IsRecycleable() = 0;

 protected:
  CanvasResource() {}
};

// Resource type for Bitmaps
class CanvasResource_Skia : public CanvasResource {
 public:
  static RefPtr<CanvasResource_Skia> Create(sk_sp<SkImage> image) {
    return AdoptRef(new CanvasResource_Skia(image));
  }

  ~CanvasResource_Skia() final {}

  // Not recyclable: Skia handles texture recycling internally and bitmaps are
  // cheap to allocate.
  bool IsRecycleable() final { return false; }

  void Abandon() final { NOTREACHED(); }

 private:
  CanvasResource_Skia(sk_sp<SkImage> image) : image_(std::move(image)) {}

  sk_sp<SkImage> image_;
};

class CanvasResource_GpuMemoryBuffer : public CanvasResource {
 public:
  static RefPtr<CanvasResource_GpuMemoryBuffer> Create(
      const IntSize& size,
      GLenum filter,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper) {
    return AdoptRef(new CanvasResource_GpuMemoryBuffer(
        size, filter, context_provider_wrapper));
  }

  ~CanvasResource_GpuMemoryBuffer() final {
    if (!context_provider_wrapper_ || !image_id_)
      return;
    auto gl = context_provider_wrapper_->ContextProvider()->ContextGL();

    if (gl) {
      GLenum target = GL_TEXTURE_RECTANGLE_ARB;
      gl->BindTexture(target, texture_id_);
      gl->ReleaseTexImage2DCHROMIUM(target, image_id_);
      gl->DestroyImageCHROMIUM(image_id_);
      gl->DeleteTextures(1, &texture_id_);
      gl->BindTexture(target, 0);
      ResetSkiaTextureBinding(context_provider_wrapper_);
    }
  }

  bool IsRecycleable() final { return IsValid(); }

  bool IsValid() { return context_provider_wrapper_ && image_id_; }

  void Abandon() final {
    image_id_ = 0;
    texture_id_ = 0;
    gpu_memory_buffer_ = nullptr;
  }

  void SetFilter(GLenum filter) { filter_ = filter; }

 private:
  CanvasResource_GpuMemoryBuffer(
      const IntSize& size,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : context_provider_wrapper_(std::move(context_provider_wrapper)) {
    if (!context_provider_wrapper_)
      return;
    auto gl = context_provider_wrapper_->ContextProvider()->ContextGL();
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager =
        Platform::Current()->GetGpuMemoryBufferManager();
    if (!gpu_memory_buffer_manager)
      return;
    gpu_memory_buffer_ = gpu_memory_buffer_manager->CreateGpuMemoryBuffer(
        gfx::Size(size.Width(), size.Height()),
        gfx::BufferFormat::RGBA_8888,  // Use format
        gfx::BufferUsage::SCANOUT, gpu::kNullSurfaceHandle);
    image_id_ = gl->CreateImageCHROMIUM(gpu_memory_buffer_->AsClientBuffer(),
                                        size.Width(), size.Height(), GL_RGBA);
    if (!image_id_) {
      gpu_memory_buffer_ = nullptr;
      return;
    }

    gl->GenTextures(1, &texture_id_);
    const GLenum target = GL_TEXTURE_RECTANGLE_ARB;
    gl->BindTexture(target, texture_id_);
    gl->TexParameteri(target, GL_TEXTURE_MAG_FILTER, filter_;
    gl->TexParameteri(target, GL_TEXTURE_MIN_FILTER, filter_;
    gl->TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->BindTexImage2DCHROMIUM(target, image_id_);

    ResetSkiaTextureBinding(context_provider_wrapper_);
  }

  std::unique_ptr<gfx::GpuMemoryBuffer> gpu_memory_buffer_;
  GLuint image_id_;
  GLuint texture_id_;
  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  GLenum filter_;
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
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider(size,
                               color_params,
                               std::move(context_provider_wrapper)) {
    if (IsGpuContextLost())
      return;
    resource_ = WTF::WrapUnique(
        new CanvasResource_GpuMemoryBuffer(size, ContextProviderWrapper()));
  }

  bool IsValid() final { return texture_id_ && !!context_provider_wrapper_; }

  bool PrepareTextureMailboxAndSwap(viz::TextureMailbox* out_mailbox) final {
    if (!IsValid())
      return false;
    auto gl = ContextGL();
    auto gr = GetGrContext();
    CHECK(gl && gr);  // IsValid() should be a sufficient guarantee

    // Context must be flushed because texture will be accessed outside of skia.
    gr->flush();

    auto mailbox = GpuMailbox();
    const GLenum target = GC3D_TEXTURE_RECTANGLE_ARB;
    gl->ProduceTextureDirectCHROMIUM(resource_->TextureId(), target,
                                     mailbox.name);

    const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
    gl->ShallowFlushCHROMIUM();
    gpu::SyncToken sync_token;
    gl->GenSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

    bool is_overlay_candidate = true;
    bool secure_output_only = false;

    *out_mailbox =
        viz::TextureMailbox(mailbox, sync_token, target, gfx::Size(size_),
                            is_overlay_candidate, secure_output_only);
    if (CanvasColorParams::ColorCorrectRenderingEnabled()) {
      gfx::ColorSpace color_space = ColorParams.GetGfxColorSpace();
      out_mailbox->set_color_space(color_space);
      gpu_memory_buffer_->SetColorSpaceForScanout(color_space);
    }

    gl->BindTexture(target, 0);

    // Because we are changing the texture binding without going through skia,
    // we must dirty the context.
    ResetSkiaTextureBinding(context_provider_wrapper_);

    return true;
  }

  //  ResourceType Type() final { return kGpuMemoryBufferResourceType; }

  // Because the SkSurface wraps an external resource, copy-on-write is not
  // possible. Therefore, taking a snapshot result in an upfront copy.
  bool IsSnapshotExpensive() const final { return true; }

 private:
  sk_sp<SkSurface> CreateSurface() final {
    if (!IsValid())
      return nullptr;
    GrGLTextureInfo texture_info;
    texture_info.fTarget = GC3D_TEXTURE_RECTANGLE_ARB;
    texture_info.fID = texture_id_;
    GrBackendTexture backend_texture(Size().Width(), Size().Height(),
                                     ColorParams().GetGrPixelConfig(),
                                     texture_info);
    return SkSurface::MakeFromBackendTextureAsRenderTarget(
        GetGrContext(), backend_texture, kTopLeft_GrSurfaceOrigin,
        ColorParams().MSAASampleCount(),
        ColorParams().GetSkColorSpaceForSkSurface(),
        ColorParams().GetSkSurfaceProps());
  }

  virtual ReftPtr<StaticBitmapImage> CreateSnapshot() {
    DCHECK(surface_);
    sk_sp<SkImage> image = surface_->makeImageSnapshot();

    return StaticBitmapImage::Create(std::move(image),
                                     ContextProviderWrapper());
  }

  virtual ReftPtr<StaticBitmapImage> CreateEphemeralSnapshot() {
    if (!IsValid())
      return nullptr;
    GrBackendTexture backend_texture(Size().Width(), Size().Height(),
                                     ColorParams().GetGrPixelConfig(),
                                     texture_info);
    sk_sp<SkImage> image = SkImage::MakeFromTexture(
        GetGrContext(), backend_texture, kTopLeft_GrSurfaceOrigin,
        ColorParams().GetSkAlphaType, ColorParams().GetSkColorSpace());
  }

  WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  std::unique_ptr<CanvasResource_GpuMemoryBuffer> resource_;
};

// CanvasResourceProvider_Texture
//==============================================================================

class CanvasResourceProvider_Texture
    : public RefCounted<CanvasResourceProvider> {
 public:
  CanvasResourceProvider_Texture(
      const IntSize& size,
      const CanvasColorParams color_params,
      WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper)
      : CanvasResourceProvider(size,
                               color_params,
                               std::move(context_provider_wrapper)) {}

  ~CanvasResourceProvider_Texture() {}

  bool IsValid() final { return surface_ && !IsGpuContextLost(); }

  unique_ptr<CanvasResource> PrepareTextureMailboxAndSwap(
      viz::TextureMailbox* out_mailbox,
      unique_ptr<CanvasResource> recycled_resource) const final {
    // Texture resources are recycled by Skia, not CanvasResourceProvider.
    DCHECK(!recycled_resource);
    DCHECK(surface_);

    if (IsGpuContextLost())
      return nullptr;

    auto gl = ContextGL();
    auto gr = GetGrContext();
    DCHECK(gl && gr);

    if (context_provider_wrapper_->ContextProvider()
            ->GetCapabilities()
            .disable_2d_canvas_copy_on_write) {
      surface_->notifyContentWillChange(SkSurface::kRetain_ContentChangeMode);
    }

    sk_sp<SkImage> image = surface_->makeImageSnapshot();
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

    *out_mailbox =
        viz::TextureMailbox(mailbox, sync_token, target, gfx::Size(size_),
                            is_overlay_candidate, secure_output_only);
    if (CanvasColorParams::ColorCorrectRenderingEnabled()) {
      gfx::ColorSpace color_space = ColorParams.GetGfxColorSpace();
      out_mailbox->set_color_space(color_space);
    }

    gl->BindTexture(GL_TEXTURE_2D, 0);

    // Because we are changing the texture binding without going through skia,
    // we must dirty the context.
    ResetSkiaTextureBinding(context_provider_wrapper_);

    return true;
  }

  bool IsSnapshotExpensive() const final { return false; }
  bool CanPrepareTextureMailbox() const final { return true; }
  uint32_t ContentUniqueID() const final { return surface_->generationID(); }

 private:
  virtual unique_ptr<PaintCanvas> CreateSurface() {
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

  bool IsSnapshotExpensive() const final { return false; }
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

CanvasResourceProvider::~CanvasResourceProvider() {
  DCHECK(!outstanding_ephemeral_snapshot_);
}

PaintCanvas* CanvasResourceProvider::Canvas() {
  // If the following DCHECK fires, it means the image return by the previous
  // call to EphemeralSnapshot is still retained, which is problematic.
  DCHECK(!outstanding_ephemeral_snapshot_);
  if (!canvas_) {
    xform_canvas_ = SkCreateColorSpaceXformCanvas(
        GetSkSurface()->getCanvas(), ColorParams().GetSkColorSpace());
    canvas_ = WTF::WrapUnique(new SkiaPaintCanvas(xform_canvas_));
    canvas->clear(ColorParams.ClearColor());
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

void CanvasResourceProvider::RecycleResource(
    unique_ptr<CanvasResource> resource) {
  recycled_resources_.push_back(std::move(resource));
}

bool CanvasResourceProvider::PrepareTextureMailbox(
    viz::TextureMailbox* out_mailbox,
    std::unique_ptr<viz::SingleReleaseCallback>* out_callback) {
  DCHECK(CanPrepareTextureMailbox());
  unique_ptr<CanvasResource> resource =
      PrepareTextureMailboxAndSwap(out_mailbox, recycled_resources_.pop_back());
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

}  // namespace blink
