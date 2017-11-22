// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/raster_implementation.h"

#include "base/logging.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/raster_texture_format_utils.h"

namespace gpu {
namespace raster {

RasterImplementation::RasterImplementation(gles2::GLES2Interface* gl,
                                           const gpu::Capabilities& caps)
    : gl_(gl),
      use_texture_storage_(caps.texture_storage),
      use_texture_storage_image_(caps.texture_storage_image) {}

RasterImplementation::~RasterImplementation() {}

GLuint64 RasterImplementation::InsertFenceSyncCHROMIUM() {
  return gl_->InsertFenceSyncCHROMIUM();
}

void RasterImplementation::GenUnverifiedSyncTokenCHROMIUM(GLuint64 fence_sync,
                                                          GLbyte* sync_token) {
  gl_->GenUnverifiedSyncTokenCHROMIUM(fence_sync, sync_token);
}

void RasterImplementation::VerifySyncTokensCHROMIUM(GLbyte** sync_tokens,
                                                    GLsizei count) {
  gl_->VerifySyncTokensCHROMIUM(sync_tokens, count);
}

void RasterImplementation::WaitSyncTokenCHROMIUM(const GLbyte* sync_token) {
  gl_->WaitSyncTokenCHROMIUM(sync_token);
}

void RasterImplementation::OrderingBarrierCHROMIUM() {
  gl_->OrderingBarrierCHROMIUM();
}

void RasterImplementation::Finish() {
  gl_->Finish();
}
void RasterImplementation::ShallowFlushCHROMIUM() {
  gl_->ShallowFlushCHROMIUM();
}

GLenum RasterImplementation::GetGraphicsResetStatusKHR() {
  return gl_->GetGraphicsResetStatusKHR();
}

void RasterImplementation::GetIntegerv(GLenum pname, GLint* params) {
  gl_->GetIntegerv(pname, params);
}

void RasterImplementation::GenTextures(GLsizei n, GLuint* textures) {
  gl_->GenTextures(n, textures);
}

void RasterImplementation::DeleteTextures(GLsizei n, const GLuint* textures) {
  gl_->DeleteTextures(n, textures);
};

void RasterImplementation::BindTexture(GLenum target, GLuint texture) {
  gl_->BindTexture(target, texture);
};

void RasterImplementation::ActiveTexture(GLenum texture) {
  gl_->ActiveTexture(texture);
}

void RasterImplementation::GenerateMipmap(GLenum target) {
  gl_->GenerateMipmap(target);
}

void RasterImplementation::TexParameteri(GLenum target,
                                         GLenum pname,
                                         GLint param) {
  gl_->TexParameteri(target, pname, param);
}

void RasterImplementation::GenMailboxCHROMIUM(GLbyte* mailbox) {
  gl_->GenMailboxCHROMIUM(mailbox);
}

void RasterImplementation::ProduceTextureDirectCHROMIUM(GLuint texture,
                                                        GLenum target,
                                                        const GLbyte* mailbox) {
  gl_->ProduceTextureDirectCHROMIUM(texture, target, mailbox);
}

GLuint RasterImplementation::CreateAndConsumeTextureCHROMIUM(
    GLenum target,
    const GLbyte* mailbox) {
  return gl_->CreateAndConsumeTextureCHROMIUM(target, mailbox);
}

GLuint RasterImplementation::CreateImageCHROMIUM(ClientBuffer buffer,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLenum internalformat) {
  return gl_->CreateImageCHROMIUM(buffer, width, height, internalformat);
}

void RasterImplementation::BindTexImage2DCHROMIUM(GLenum target,
                                                  GLint imageId) {
  gl_->BindTexImage2DCHROMIUM(target, imageId);
}

void RasterImplementation::ReleaseTexImage2DCHROMIUM(GLenum target,
                                                     GLint imageId) {
  gl_->ReleaseTexImage2DCHROMIUM(target, imageId);
}

void RasterImplementation::DestroyImageCHROMIUM(GLuint image_id) {
  gl_->DestroyImageCHROMIUM(image_id);
}

void RasterImplementation::CompressedTexImage2D(GLenum target,
                                                GLint level,
                                                GLenum internalformat,
                                                GLsizei width,
                                                GLsizei height,
                                                GLint border,
                                                GLsizei imageSize,
                                                const void* data) {
  gl_->CompressedTexImage2D(target, level, internalformat, width, height,
                            border, imageSize, data);
}

void RasterImplementation::TexSubImage2D(GLenum target,
                                         GLint level,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLenum type,
                                         const void* pixels) {
  gl_->TexSubImage2D(target, level, xoffset, yoffset, width, height, format,
                     type, pixels);
}

void RasterImplementation::TexStorageForRaster(GLenum target,
                                               TextureFormat format,
                                               GLsizei width,
                                               GLsizei height,
                                               RasterTexStorageFlags flags) {
  (void)use_texture_storage_;

  if (flags & kOverlay) {
    DCHECK(use_texture_storage_image_);
    gl_->TexStorage2DImageCHROMIUM(target, TextureStorageFormat(format),
                                   GL_SCANOUT_CHROMIUM, width, height);

    // TODO(vmiura): Set color space.
    // if (color_space_.IsValid()) {
    //  gl->SetColorSpaceMetadataCHROMIUM(
    //      texture_id, reinterpret_cast<GLColorSpace>(&color_space_));
    //}
  } else if (use_texture_storage_) {
    GLint levels = 1;
    gl_->TexStorage2DEXT(target, levels, TextureStorageFormat(format), width,
                         height);
  } else {
    gl_->TexImage2D(target, 0, GLInternalFormat(format), width, height, 0,
                    GLDataFormat(format), GLDataType(format), nullptr);
  }
}

void RasterImplementation::SetColorSpaceMetadataCHROMIUM(
    GLuint texture_id,
    GLColorSpace color_space) {
  gl_->SetColorSpaceMetadataCHROMIUM(texture_id, color_space);
}

void RasterImplementation::BeginRasterCHROMIUM(
    GLuint texture_id,
    GLuint sk_color,
    GLuint msaa_sample_count,
    GLboolean can_use_lcd_text,
    GLboolean use_distance_field_text,
    GLint pixel_config) {
  gl_->BeginRasterCHROMIUM(texture_id, sk_color, msaa_sample_count,
                           can_use_lcd_text, use_distance_field_text,
                           pixel_config);
};

void RasterImplementation::RasterCHROMIUM(const cc::DisplayItemList* list,
                                          GLint translate_x,
                                          GLint translate_y,
                                          GLint clip_x,
                                          GLint clip_y,
                                          GLint clip_w,
                                          GLint clip_h,
                                          GLfloat post_translate_x,
                                          GLfloat post_translate_y,
                                          GLfloat post_scale) {
  gl_->RasterCHROMIUM(list, translate_x, translate_y, clip_x, clip_y, clip_w,
                      clip_h, post_translate_x, post_translate_y, post_scale);
}

void RasterImplementation::EndRasterCHROMIUM() {
  gl_->EndRasterCHROMIUM();
}

}  // namespace raster
}  // namespace gpu
