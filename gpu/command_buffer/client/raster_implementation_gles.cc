// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/raster_implementation_gles.h"

#include "base/logging.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"

namespace gpu {
namespace raster {

static GLenum GetImageTextureTarget(const gpu::Capabilities& caps,
                                    gfx::BufferUsage usage,
                                    viz::ResourceFormat format) {
  gfx::BufferFormat buffer_format = viz::BufferFormat(format);
  return GetBufferTextureTarget(usage, buffer_format, caps);
}

RasterImplementationGLES::Texture* RasterImplementationGLES::GetTexture(
    GLuint texture_id,
    bool validate_binding) {
  auto it = texture_info_.find(texture_id);
  if (it == texture_info_.end())
    return nullptr;

  if (validate_binding && it->second.target == GL_NONE)
    return nullptr;

  return &it->second;
}

RasterImplementationGLES::Texture::Texture(GLuint id,
                                           GLenum target,
                                           bool use_buffer,
                                           gfx::BufferUsage buffer_usage,
                                           viz::ResourceFormat format)
    : id(id),
      target(target),
      use_buffer(use_buffer),
      buffer_usage(buffer_usage),
      format(format) {}

RasterImplementationGLES::Texture* RasterImplementationGLES::EnsureTextureBound(
    RasterImplementationGLES::Texture* texture) {
  if (texture /*&& bound_texture_ != texture*/) {
    bound_texture_ = texture;
    gl_->BindTexture(texture->target, texture->id);
    return texture;
  }
  return nullptr;
}

RasterImplementationGLES::RasterImplementationGLES(
    gles2::GLES2Interface* gl,
    const gpu::Capabilities& caps)
    : gl_(gl),
      caps_(caps),
      use_texture_storage_(caps.texture_storage),
      use_texture_storage_image_(caps.texture_storage_image) {}

RasterImplementationGLES::~RasterImplementationGLES() {}

void RasterImplementationGLES::Finish() {
  gl_->Finish();
}

void RasterImplementationGLES::Flush() {
  gl_->Flush();
}

void RasterImplementationGLES::ShallowFlushCHROMIUM() {
  gl_->ShallowFlushCHROMIUM();
}

void RasterImplementationGLES::OrderingBarrierCHROMIUM() {
  gl_->OrderingBarrierCHROMIUM();
}

void RasterImplementationGLES::GenSyncTokenCHROMIUM(GLbyte* sync_token) {
  gl_->GenSyncTokenCHROMIUM(sync_token);
}

void RasterImplementationGLES::GenUnverifiedSyncTokenCHROMIUM(
    GLbyte* sync_token) {
  gl_->GenUnverifiedSyncTokenCHROMIUM(sync_token);
}

void RasterImplementationGLES::VerifySyncTokensCHROMIUM(GLbyte** sync_tokens,
                                                        GLsizei count) {
  gl_->VerifySyncTokensCHROMIUM(sync_tokens, count);
}

void RasterImplementationGLES::WaitSyncTokenCHROMIUM(const GLbyte* sync_token) {
  gl_->WaitSyncTokenCHROMIUM(sync_token);
}

GLenum RasterImplementationGLES::GetError() {
  return gl_->GetError();
}

GLenum RasterImplementationGLES::GetGraphicsResetStatusKHR() {
  return gl_->GetGraphicsResetStatusKHR();
}

void RasterImplementationGLES::GetIntegerv(GLenum pname, GLint* params) {
  gl_->GetIntegerv(pname, params);
}

void RasterImplementationGLES::LoseContextCHROMIUM(GLenum current,
                                                   GLenum other) {
  gl_->LoseContextCHROMIUM(current, other);
}

void RasterImplementationGLES::GenQueriesEXT(GLsizei n, GLuint* queries) {
  gl_->GenQueriesEXT(n, queries);
}

void RasterImplementationGLES::DeleteQueriesEXT(GLsizei n,
                                                const GLuint* queries) {
  gl_->DeleteQueriesEXT(n, queries);
}

void RasterImplementationGLES::BeginQueryEXT(GLenum target, GLuint id) {
  gl_->BeginQueryEXT(target, id);
}

void RasterImplementationGLES::EndQueryEXT(GLenum target) {
  gl_->EndQueryEXT(target);
}

void RasterImplementationGLES::GetQueryObjectuivEXT(GLuint id,
                                                    GLenum pname,
                                                    GLuint* params) {
  gl_->GetQueryObjectuivEXT(id, pname, params);
}

GLuint RasterImplementationGLES::CreateTexture(bool use_buffer,
                                               gfx::BufferUsage buffer_usage,
                                               viz::ResourceFormat format) {
  GLuint texture_id = 0;
  gl_->GenTextures(1, &texture_id);

  if (texture_id) {
    GLenum target = use_buffer
                        ? GetImageTextureTarget(caps_, buffer_usage, format)
                        : GL_TEXTURE_2D;
    texture_info_.emplace(std::make_pair(
        texture_id,
        Texture(texture_id, target, use_buffer, buffer_usage, format)));
  }
  return texture_id;
}

void RasterImplementationGLES::DeleteTextures(GLsizei n,
                                              const GLuint* textures) {
  if (n < 0) {
    // TODO(vmiura): set error
    return;
  }

  for (GLsizei i = 0; i < n; i++) {
    // TODO(vmiura): Error checking.
    if (bound_texture_ && bound_texture_->id == textures[i])
      bound_texture_ = nullptr;
    texture_info_.erase(textures[i]);
  }

  gl_->DeleteTextures(n, textures);
};

void RasterImplementationGLES::SetColorSpaceMetadata(GLuint texture_id,
                                                     GLColorSpace color_space) {
  if (Texture* texture = GetTexture(texture_id, true))
    gl_->SetColorSpaceMetadataCHROMIUM(texture->id, color_space);
}

void RasterImplementationGLES::TexParameteri(GLuint texture_id,
                                             GLenum pname,
                                             GLint param) {
  if (Texture* texture = EnsureTextureBound(GetTexture(texture_id, true)))
    gl_->TexParameteri(texture->target, pname, param);
}

void RasterImplementationGLES::GenMailbox(GLbyte* mailbox) {
  gl_->GenMailboxCHROMIUM(mailbox);
}

void RasterImplementationGLES::ProduceTextureDirect(GLuint texture_id,
                                                    const GLbyte* mailbox) {
  Texture* texture = GetTexture(texture_id, false);
  if (!texture)
    return;

  gl_->ProduceTextureDirectCHROMIUM(texture_id, mailbox);
}

GLuint RasterImplementationGLES::CreateAndConsumeTexture(
    bool use_buffer,
    gfx::BufferUsage buffer_usage,
    viz::ResourceFormat format,
    const GLbyte* mailbox) {
  GLuint texture_id = gl_->CreateAndConsumeTextureCHROMIUM(mailbox);

  if (texture_id) {
    GLenum target = use_buffer
                        ? GetImageTextureTarget(caps_, buffer_usage, format)
                        : GL_TEXTURE_2D;
    texture_info_.emplace(std::make_pair(
        texture_id,
        Texture(texture_id, target, use_buffer, buffer_usage, format)));
  }

  return texture_id;
}

GLuint RasterImplementationGLES::CreateImageCHROMIUM(ClientBuffer buffer,
                                                     GLsizei width,
                                                     GLsizei height,
                                                     GLenum internalformat) {
  return gl_->CreateImageCHROMIUM(buffer, width, height, internalformat);
}

void RasterImplementationGLES::BindTexImage2DCHROMIUM(GLuint texture_id,
                                                      GLint image_id) {
  if (Texture* texture = EnsureTextureBound(GetTexture(texture_id, true)))
    gl_->BindTexImage2DCHROMIUM(texture->target, image_id);
}

void RasterImplementationGLES::ReleaseTexImage2DCHROMIUM(GLuint texture_id,
                                                         GLint image_id) {
  if (Texture* texture = EnsureTextureBound(GetTexture(texture_id, true)))
    gl_->ReleaseTexImage2DCHROMIUM(texture->target, image_id);
}

void RasterImplementationGLES::DestroyImageCHROMIUM(GLuint image_id) {
  gl_->DestroyImageCHROMIUM(image_id);
}

void RasterImplementationGLES::TexStorage2D(GLuint texture_id,
                                            GLint levels,
                                            GLsizei width,
                                            GLsizei height) {
  Texture* texture = EnsureTextureBound(GetTexture(texture_id, true));
  if (!texture)
    return;

  if (texture->use_buffer) {
    DCHECK(use_texture_storage_image_);
    DCHECK(levels == 1);
    gl_->TexStorage2DImageCHROMIUM(texture->target,
                                   viz::TextureStorageFormat(texture->format),
                                   GL_SCANOUT_CHROMIUM, width, height);
  } else if (use_texture_storage_) {
    gl_->TexStorage2DEXT(texture->target, levels,
                         viz::TextureStorageFormat(texture->format), width,
                         height);
  } else {
    // TODO(vmiura): Allocate all levels.
    gl_->TexImage2D(texture->target, 0, viz::GLInternalFormat(texture->format),
                    width, height, 0, viz::GLDataFormat(texture->format),
                    viz::GLDataType(texture->format), nullptr);
  }
}

void RasterImplementationGLES::CopySubTexture(GLuint source_id,
                                              GLuint dest_id,
                                              GLint xoffset,
                                              GLint yoffset,
                                              GLint x,
                                              GLint y,
                                              GLsizei width,
                                              GLsizei height) {
  Texture* source = GetTexture(source_id, true);
  Texture* dest = GetTexture(dest_id, true);

  if (!source || !dest) {
    LOG(ERROR) << "Failed to get source or dest " << source << " " << dest;
    return;
  }

  gl_->CopySubTextureCHROMIUM(source->id, 0, dest->target, dest->id, 0, xoffset,
                              yoffset, x, y, width, height, false, false,
                              false);
}

void RasterImplementationGLES::CompressedCopyTextureCHROMIUM(GLuint source_id,
                                                             GLuint dest_id) {
  Texture* source = GetTexture(source_id, true);
  Texture* dest = GetTexture(dest_id, true);

  if (!source || !dest) {
    LOG(ERROR) << "Failed to get source or dest " << source << " " << dest;
    return;
  }

  gl_->CompressedCopyTextureCHROMIUM(source->id, dest->id);
}

void RasterImplementationGLES::InitializeDiscardableTextureCHROMIUM(
    GLuint texture_id) {
  if (Texture* texture = GetTexture(texture_id, true))
    gl_->InitializeDiscardableTextureCHROMIUM(texture->id);
}

void RasterImplementationGLES::UnlockDiscardableTextureCHROMIUM(
    GLuint texture_id) {
  if (Texture* texture = GetTexture(texture_id, true))
    gl_->UnlockDiscardableTextureCHROMIUM(texture->id);
}

bool RasterImplementationGLES::LockDiscardableTextureCHROMIUM(
    GLuint texture_id) {
  if (Texture* texture = GetTexture(texture_id, true))
    return gl_->LockDiscardableTextureCHROMIUM(texture->id);
  return false;
}

void RasterImplementationGLES::BeginRasterCHROMIUM(
    GLuint texture_id,
    GLuint sk_color,
    GLuint msaa_sample_count,
    GLboolean can_use_lcd_text,
    GLboolean use_distance_field_text,
    GLint pixel_config) {
  if (Texture* texture = GetTexture(texture_id, true)) {
    gl_->BeginRasterCHROMIUM(texture->id, sk_color, msaa_sample_count,
                             can_use_lcd_text, use_distance_field_text,
                             pixel_config);
  }
};

void RasterImplementationGLES::RasterCHROMIUM(
    const cc::DisplayItemList* list,
    cc::ImageProvider* provider,
    const gfx::Vector2d& translate,
    const gfx::Rect& playback_rect,
    const gfx::Vector2dF& post_translate,
    GLfloat post_scale) {
  gl_->RasterCHROMIUM(list, provider, translate, playback_rect, post_translate,
                      post_scale);
}

void RasterImplementationGLES::EndRasterCHROMIUM() {
  gl_->EndRasterCHROMIUM();
}

void RasterImplementationGLES::BeginGpuRaster() {
  // TODO(alokp): Use a trace macro to push/pop markers.
  // Using push/pop functions directly incurs cost to evaluate function
  // arguments even when tracing is disabled.
  gl_->TraceBeginCHROMIUM("BeginGpuRaster", "GpuRasterization");
}

void RasterImplementationGLES::EndGpuRaster() {
  // Restore default GL unpack alignment.  TextureUploader expects this.
  gl_->PixelStorei(GL_UNPACK_ALIGNMENT, 4);

  // TODO(alokp): Use a trace macro to push/pop markers.
  // Using push/pop functions directly incurs cost to evaluate function
  // arguments even when tracing is disabled.
  gl_->TraceEndCHROMIUM();
}

}  // namespace raster
}  // namespace gpu
