// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/gpu_raster_buffer_provider.h"

#include <stdint.h>

#include <algorithm>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/histograms.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_recorder.h"
#include "cc/raster/raster_source.h"
#include "cc/raster/scoped_gpu_raster.h"
#include "cc/resources/resource.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/gpu/raster_context_provider.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "skia/ext/texture_handle.h"
#include "third_party/skia/include/core/SkMultiPictureDraw.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gfx/geometry/axis_transform2d.h"

namespace cc {

class UnpremultiplyEffect {
 public:
  UnpremultiplyEffect(gpu::gles2::GLES2Interface* gl) {
    LOG(ERROR) << "HI";
    const char* kVertexShader =
        "attribute vec4 in_position;\n"
        "void main() {\n"
        "  gl_Position = in_position;\n"
        "}\n";

    const char* kFragmentShader =
        "precision highp float;\n"
        "uniform sampler2D tex;\n"
        "uniform vec2 tex_size;\n"
        "void main() {\n"
        "  vec2 tex_coord = gl_FragCoord.xy / tex_size;\n"
        "  tex_coord.y = 1.0 - tex_coord.y;\n"
        "  vec4 in_color = texture2D(tex, tex_coord);\n"
        "  gl_FragColor = vec4((in_color.rgb + vec3(0.5/16.0, 0.5/16.0, "
        "0.5/16.0)) * (1.0 / in_color.a), in_color.a);\n"
        "}\n";

    const GLfloat kVertices[] = {-1.0f, 1.0f, -1.0f, -1.0f,
                                 1.0f,  1.0f, 1.0f,  -1.0f};

    // gl->EnableVertexAttribArray(0);
    // gl->GenVertexArraysOES(1, &vao_id_);
    // gl->BindVertexArrayOES(vao_id_);
    gl->GenBuffers(1, &vbo_id_);
    gl->BindBuffer(GL_ARRAY_BUFFER, vbo_id_);
    gl->BufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), kVertices,
                   GL_STATIC_DRAW);
    // gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint vsh_id = gl->CreateShader(GL_VERTEX_SHADER);
    gl->ShaderSource(vsh_id, 1, &kVertexShader, nullptr);
    gl->CompileShader(vsh_id);

    GLint is_compiled = GL_FALSE;
    gl->GetShaderiv(vsh_id, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE) {
      GLint max_length = 0;
      gl->GetShaderiv(vsh_id, GL_INFO_LOG_LENGTH, &max_length);

      // The maxLength includes the NULL character
      std::vector<GLchar> info_log(max_length);
      GLint real_length = 0;
      gl->GetShaderInfoLog(vsh_id, max_length, &real_length, info_log.data());

      LOG(ERROR) << std::string(info_log.data(), real_length);
      return;
    }
    GLuint fsh_id = gl->CreateShader(GL_FRAGMENT_SHADER);
    gl->ShaderSource(fsh_id, 1, &kFragmentShader, nullptr);
    gl->CompileShader(fsh_id);
    is_compiled = GL_FALSE;
    gl->GetShaderiv(fsh_id, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE) {
      GLint max_length = 0;
      gl->GetShaderiv(fsh_id, GL_INFO_LOG_LENGTH, &max_length);

      // The maxLength includes the NULL character
      std::vector<GLchar> info_log(max_length);
      GLint real_length = 0;
      gl->GetShaderInfoLog(fsh_id, max_length, &real_length, &info_log[0]);

      LOG(ERROR) << std::string(info_log.data(), real_length);
      return;
    }

    program_id_ = gl->CreateProgram();
    gl->AttachShader(program_id_, vsh_id);
    gl->AttachShader(program_id_, fsh_id);
    gl->LinkProgram(program_id_);
    texture_id_ = gl->GetUniformLocation(program_id_, "tex");
    size_id_ = gl->GetUniformLocation(program_id_, "tex_size");

    gl->DetachShader(program_id_, vsh_id);
    gl->DetachShader(program_id_, fsh_id);
    gl->GenFramebuffers(1, &framebuffer_id_);
  }
  void Unpremultiply(gpu::gles2::GLES2Interface* gl,
                     GLuint in_texture,
                     GLuint out_texture,
                     GLfloat width,
                     GLfloat height) {
    gl->EnableVertexAttribArray(0);
    gl->BindBuffer(GL_ARRAY_BUFFER, vbo_id_);
    gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    // gl->BindVertexArrayOES(vao_id_);
    gl->UseProgram(program_id_);
    gl->ActiveTexture(GL_TEXTURE0);
    gl->BindTexture(GL_TEXTURE_2D, in_texture);
    gl->ActiveTexture(GL_TEXTURE1);
    gl->BindTexture(GL_TEXTURE_2D, 0);
    gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, out_texture, 0);
    // gl->ClearColor(1,1,0,1);
    // gl->Clear(GL_COLOR_BUFFER_BIT);
    gl->Viewport(0, 0, width, height);
    gl->Disable(GL_CULL_FACE);
    gl->Disable(GL_STENCIL_TEST);
    gl->Disable(GL_BLEND);
    gl->Uniform1i(texture_id_, 0);
    gl->Uniform2f(size_id_, width, height);
    gl->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
    // gl->DeleteFramebuffers(1, &framebuffer_id);
  }

 private:
  GLuint program_id_;
  // GLuint vao_id_;
  GLuint texture_id_ = 0;
  GLuint size_id_ = 0;
  GLuint framebuffer_id_ = 0;
  GLuint vbo_id_;
};

namespace {

static void RasterizeSourceOOP(
    const RasterSource* raster_source,
    bool resource_has_previous_content,
    const gfx::Size& resource_size,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& playback_rect,
    const gfx::AxisTransform2d& transform,
    const RasterSource::PlaybackSettings& playback_settings,
    viz::RasterContextProvider* context_provider,
    LayerTreeResourceProvider::ScopedWriteLockRaster* resource_lock,
    bool use_distance_field_text,
    int msaa_sample_count) {
  gpu::raster::RasterInterface* ri = context_provider->RasterInterface();
  GLuint texture_id = resource_lock->ConsumeTexture(ri);

  ri->BeginRasterCHROMIUM(texture_id, raster_source->background_color(),
                          msaa_sample_count, playback_settings.use_lcd_text,
                          use_distance_field_text,
                          resource_lock->PixelConfig());
  // TODO(enne): need to pass color space into this function as well.
  float recording_to_raster_scale =
      transform.scale() / raster_source->recording_scale_factor();
  ri->RasterCHROMIUM(raster_source->GetDisplayItemList().get(),
                     playback_settings.image_provider,
                     raster_full_rect.OffsetFromOrigin(), playback_rect,
                     transform.translation(), recording_to_raster_scale);
  ri->EndRasterCHROMIUM();

  ri->DeleteTextures(1, &texture_id);
}

// The following class is needed to correctly reset GL state when rendering to
// SkCanvases with a GrContext on a RasterInterface enabled context.
class ScopedGrContextAccess {
 public:
  explicit ScopedGrContextAccess(viz::RasterContextProvider* context_provider)
      : context_provider_(context_provider) {
    gpu::raster::RasterInterface* ri = context_provider_->RasterInterface();
    ri->BeginGpuRaster();

    class GrContext* gr_context = context_provider_->GrContext();
    gr_context->resetContext();
  }
  ~ScopedGrContextAccess() {
    gpu::raster::RasterInterface* ri = context_provider_->RasterInterface();
    ri->EndGpuRaster();
  }

 private:
  viz::RasterContextProvider* context_provider_;
};

static void RasterizeSource(
    const RasterSource* raster_source,
    bool resource_has_previous_content,
    const gfx::Size& resource_size,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& playback_rect,
    const gfx::AxisTransform2d& transform,
    const RasterSource::PlaybackSettings& playback_settings,
    viz::RasterContextProvider* context_provider,
    LayerTreeResourceProvider::ScopedWriteLockRaster* resource_lock,
    bool use_distance_field_text,
    int msaa_sample_count,
    UnpremultiplyEffect* unpremultiply_effect) {
  ScopedGrContextAccess gr_context_access(context_provider);

  gpu::raster::RasterInterface* ri = context_provider->RasterInterface();
  GLuint texture_id = resource_lock->ConsumeTexture(ri);

  {
    SkImageInfo n32Info = SkImageInfo::MakeN32Premul(
        resource_lock->size().width(), resource_lock->size().height());
    sk_sp<SkSurface> surface = SkSurface::MakeRenderTarget(
        context_provider->GrContext(), SkBudgeted::kYes, n32Info);

    // Allocating an SkSurface will fail after a lost context.  Pretend we
    // rasterized, as the contents of the resource don't matter anymore.
    if (!surface) {
      DLOG(ERROR) << "Failed to allocate raster surface";
      return;
    }

    SkCanvas* canvas = surface->getCanvas();

    // As an optimization, inform Skia to discard when not doing partial raster.
    if (raster_full_rect == playback_rect)
      canvas->discard();

    raster_source->PlaybackToCanvas(
        canvas, resource_lock->color_space_for_raster(), raster_full_rect,
        playback_rect, transform, playback_settings);

    GrBackendObject handle =
        surface->getTextureHandle(SkSurface::kFlushRead_BackendHandleAccess);
    const GrGLTextureInfo* info =
        skia::GrBackendObjectToGrGLTextureInfo(handle);
    context_provider->GrContext()->flush();
    unpremultiply_effect->Unpremultiply(
        static_cast<viz::ContextProvider*>(
            static_cast<ui::ContextProviderCommandBuffer*>(context_provider))
            ->ContextGL(),
        info->fID, texture_id, resource_lock->size().width(),
        resource_lock->size().height());
  }

  ri->DeleteTextures(1, &texture_id);
}

}  // namespace

GpuRasterBufferProvider::RasterBufferImpl::RasterBufferImpl(
    GpuRasterBufferProvider* client,
    LayerTreeResourceProvider* resource_provider,
    viz::ResourceId resource_id,
    bool resource_has_previous_content)
    : client_(client),
      lock_(resource_provider, resource_id),
      resource_has_previous_content_(resource_has_previous_content) {
  client_->pending_raster_buffers_.insert(this);
  lock_.CreateMailbox();
}

GpuRasterBufferProvider::RasterBufferImpl::~RasterBufferImpl() {
  client_->pending_raster_buffers_.erase(this);
}

void GpuRasterBufferProvider::RasterBufferImpl::Playback(
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    uint64_t new_content_id,
    const gfx::AxisTransform2d& transform,
    const RasterSource::PlaybackSettings& playback_settings) {
  TRACE_EVENT0("cc", "GpuRasterBuffer::Playback");
  client_->PlaybackOnWorkerThread(&lock_, sync_token_,
                                  resource_has_previous_content_, raster_source,
                                  raster_full_rect, raster_dirty_rect,
                                  new_content_id, transform, playback_settings);
}

GpuRasterBufferProvider::GpuRasterBufferProvider(
    viz::ContextProvider* compositor_context_provider,
    viz::RasterContextProvider* worker_context_provider,
    LayerTreeResourceProvider* resource_provider,
    bool use_distance_field_text,
    int gpu_rasterization_msaa_sample_count,
    viz::ResourceFormat preferred_tile_format,
    bool enable_oop_rasterization)
    : compositor_context_provider_(compositor_context_provider),
      worker_context_provider_(worker_context_provider),
      resource_provider_(resource_provider),
      use_distance_field_text_(use_distance_field_text),
      msaa_sample_count_(gpu_rasterization_msaa_sample_count),
      preferred_tile_format_(preferred_tile_format),
      enable_oop_rasterization_(enable_oop_rasterization) {
  DCHECK(compositor_context_provider);
  DCHECK(worker_context_provider);
}

GpuRasterBufferProvider::~GpuRasterBufferProvider() {
  DCHECK(pending_raster_buffers_.empty());
}

std::unique_ptr<RasterBuffer> GpuRasterBufferProvider::AcquireBufferForRaster(
    const ResourcePool::InUsePoolResource& resource,
    uint64_t resource_content_id,
    uint64_t previous_content_id) {
  bool resource_has_previous_content =
      resource_content_id && resource_content_id == previous_content_id;
  return std::make_unique<RasterBufferImpl>(this, resource_provider_,
                                            resource.gpu_backing_resource_id(),
                                            resource_has_previous_content);
}

void GpuRasterBufferProvider::OrderingBarrier() {
  TRACE_EVENT0("cc", "GpuRasterBufferProvider::OrderingBarrier");

  gpu::gles2::GLES2Interface* gl = compositor_context_provider_->ContextGL();
  gpu::SyncToken sync_token =
      LayerTreeResourceProvider::GenerateSyncTokenHelper(gl);
  for (RasterBufferImpl* buffer : pending_raster_buffers_)
    buffer->set_sync_token(sync_token);
  pending_raster_buffers_.clear();
}

void GpuRasterBufferProvider::Flush() {
  compositor_context_provider_->ContextSupport()->FlushPendingWork();
}

viz::ResourceFormat GpuRasterBufferProvider::GetResourceFormat(
    bool must_support_alpha) const {
  if (resource_provider_->IsRenderBufferFormatSupported(
          preferred_tile_format_) &&
      (DoesResourceFormatSupportAlpha(preferred_tile_format_) ||
       !must_support_alpha)) {
    return preferred_tile_format_;
  }

  return resource_provider_->best_render_buffer_format();
}

bool GpuRasterBufferProvider::IsResourceSwizzleRequired(
    bool must_support_alpha) const {
  // This doesn't require a swizzle because we rasterize to the correct format.
  return false;
}

bool GpuRasterBufferProvider::CanPartialRasterIntoProvidedResource() const {
  // Partial raster doesn't support MSAA, as the MSAA resolve is unaware of clip
  // rects.
  // TODO(crbug.com/629683): See if we can work around this limitation.
  return msaa_sample_count_ == 0;
}

bool GpuRasterBufferProvider::IsResourceReadyToDraw(
    const ResourcePool::InUsePoolResource& resource) const {
  gpu::SyncToken sync_token = resource_provider_->GetSyncTokenForResources(
      {resource.gpu_backing_resource_id()});
  if (!sync_token.HasData())
    return true;

  // IsSyncTokenSignaled is thread-safe, no need for worker context lock.
  return worker_context_provider_->ContextSupport()->IsSyncTokenSignaled(
      sync_token);
}

uint64_t GpuRasterBufferProvider::SetReadyToDrawCallback(
    const std::vector<const ResourcePool::InUsePoolResource*>& resources,
    const base::Closure& callback,
    uint64_t pending_callback_id) const {
  std::vector<viz::ResourceId> resource_ids;
  resource_ids.reserve(resources.size());
  for (auto* resource : resources)
    resource_ids.push_back(resource->gpu_backing_resource_id());
  gpu::SyncToken sync_token =
      resource_provider_->GetSyncTokenForResources(resource_ids);
  uint64_t callback_id = sync_token.release_count();
  DCHECK_NE(callback_id, 0u);

  // If the callback is different from the one the caller is already waiting on,
  // pass the callback through to SignalSyncToken. Otherwise the request is
  // redundant.
  if (callback_id != pending_callback_id) {
    // Use the compositor context because we want this callback on the impl
    // thread.
    compositor_context_provider_->ContextSupport()->SignalSyncToken(sync_token,
                                                                    callback);
  }

  return callback_id;
}

void GpuRasterBufferProvider::Shutdown() {
  pending_raster_buffers_.clear();
}

void GpuRasterBufferProvider::PlaybackOnWorkerThread(
    LayerTreeResourceProvider::ScopedWriteLockRaster* resource_lock,
    const gpu::SyncToken& sync_token,
    bool resource_has_previous_content,
    const RasterSource* raster_source,
    const gfx::Rect& raster_full_rect,
    const gfx::Rect& raster_dirty_rect,
    uint64_t new_content_id,
    const gfx::AxisTransform2d& transform,
    const RasterSource::PlaybackSettings& playback_settings) {
  viz::RasterContextProvider::ScopedRasterContextLock scoped_context(
      worker_context_provider_);
  gpu::raster::RasterInterface* ri = scoped_context.RasterInterface();
  DCHECK(ri);

  // Synchronize with compositor. Nop if sync token is empty.
  ri->WaitSyncTokenCHROMIUM(sync_token.GetConstData());

  gfx::Rect playback_rect = raster_full_rect;
  if (false && resource_has_previous_content) {
    playback_rect.Intersect(raster_dirty_rect);
  }
  DCHECK(!playback_rect.IsEmpty())
      << "Why are we rastering a tile that's not dirty?";

  if (!unpremultiply_effect_) {
    unpremultiply_effect_.reset(new UnpremultiplyEffect(
        static_cast<viz::ContextProvider*>(
            static_cast<ui::ContextProviderCommandBuffer*>(
                worker_context_provider_))
            ->ContextGL()));
  }

  // Log a histogram of the percentage of pixels that were saved due to
  // partial raster.
  const char* client_name = GetClientNameForMetrics();
  float full_rect_size = raster_full_rect.size().GetArea();
  if (full_rect_size > 0 && client_name) {
    float fraction_partial_rastered =
        static_cast<float>(playback_rect.size().GetArea()) / full_rect_size;
    float fraction_saved = 1.0f - fraction_partial_rastered;
    UMA_HISTOGRAM_PERCENTAGE(
        base::StringPrintf("Renderer4.%s.PartialRasterPercentageSaved.Gpu",
                           client_name),
        100.0f * fraction_saved);
  }

  if (enable_oop_rasterization_) {
    RasterizeSourceOOP(raster_source, resource_has_previous_content,
                       resource_lock->size(), raster_full_rect, playback_rect,
                       transform, playback_settings, worker_context_provider_,
                       resource_lock, use_distance_field_text_,
                       msaa_sample_count_);
  } else {
    RasterizeSource(raster_source, resource_has_previous_content,
                    resource_lock->size(), raster_full_rect, playback_rect,
                    transform, playback_settings, worker_context_provider_,
                    resource_lock, use_distance_field_text_, msaa_sample_count_,
                    unpremultiply_effect_.get());
  }

  // Generate sync token for cross context synchronization.
  resource_lock->set_sync_token(
      LayerTreeResourceProvider::GenerateSyncTokenHelper(ri));
}

}  // namespace cc
