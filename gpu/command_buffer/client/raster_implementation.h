// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_RASTER_IMPLEMENTATION_H_
#define GPU_COMMAND_BUFFER_CLIENT_RASTER_IMPLEMENTATION_H_

#include "base/macros.h"
#include "gles2_impl_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "gpu/command_buffer/common/capabilities.h"

namespace gpu {
namespace raster {

struct Capabilities;

class GLES2_IMPL_EXPORT RasterImplementation : public RasterInterface {
 public:
  RasterImplementation(gles2::GLES2Interface* gl,
                       const gpu::Capabilities& caps);
  ~RasterImplementation() override;

  GLuint64 InsertFenceSyncCHROMIUM() override;
  void GenUnverifiedSyncTokenCHROMIUM(GLuint64 fence_sync,
                                      GLbyte* sync_token) override;
  void VerifySyncTokensCHROMIUM(GLbyte** sync_tokens, GLsizei count) override;
  void WaitSyncTokenCHROMIUM(const GLbyte* sync_token) override;
  void OrderingBarrierCHROMIUM() override;
  void Finish() override;
  void ShallowFlushCHROMIUM() override;

  GLenum GetGraphicsResetStatusKHR() override;
  void GetIntegerv(GLenum pname, GLint* params) override;

  void GenTextures(GLsizei n, GLuint* textures) override;
  void DeleteTextures(GLsizei n, const GLuint* textures) override;
  void BindTexture(GLenum target, GLuint texture) override;
  void ActiveTexture(GLenum texture) override;

  void GenerateMipmap(GLenum target) override;
  void TexParameteri(GLenum target, GLenum pname, GLint param) override;

  void GenMailboxCHROMIUM(GLbyte* mailbox) override;
  void ProduceTextureDirectCHROMIUM(GLuint texture,
                                    GLenum target,
                                    const GLbyte* mailbox) override;
  GLuint CreateAndConsumeTextureCHROMIUM(GLenum target,
                                         const GLbyte* mailbox) override;

  GLuint CreateImageCHROMIUM(ClientBuffer buffer,
                             GLsizei width,
                             GLsizei height,
                             GLenum internalformat) override;
  void BindTexImage2DCHROMIUM(GLenum target, GLint imageId) override;
  void ReleaseTexImage2DCHROMIUM(GLenum target, GLint imageId) override;
  void DestroyImageCHROMIUM(GLuint image_id) override;

  void CompressedTexImage2D(GLenum target,
                            GLint level,
                            GLenum internalformat,
                            GLsizei width,
                            GLsizei height,
                            GLint border,
                            GLsizei imageSize,
                            const void* data) override;
  void TexSubImage2D(GLenum target,
                     GLint level,
                     GLint xoffset,
                     GLint yoffset,
                     GLsizei width,
                     GLsizei height,
                     GLenum format,
                     GLenum type,
                     const void* pixels) override;

  void TexStorageForRaster(GLenum target,
                           TextureFormat format,
                           GLsizei width,
                           GLsizei height,
                           RasterTexStorageFlags flags) override;

  void SetColorSpaceMetadataCHROMIUM(GLuint texture_id,
                                     GLColorSpace color_space) override;

  void BeginRasterCHROMIUM(GLuint texture_id,
                           GLuint sk_color,
                           GLuint msaa_sample_count,
                           GLboolean can_use_lcd_text,
                           GLboolean use_distance_field_text,
                           GLint pixel_config) override;
  void RasterCHROMIUM(const cc::DisplayItemList* list,
                      GLint translate_x,
                      GLint translate_y,
                      GLint clip_x,
                      GLint clip_y,
                      GLint clip_w,
                      GLint clip_h,
                      GLfloat post_translate_x,
                      GLfloat post_translate_y,
                      GLfloat post_scale) override;
  void EndRasterCHROMIUM() override;

 private:
  gles2::GLES2Interface* gl_;
  bool use_texture_storage_;
  bool use_texture_storage_image_;

  DISALLOW_COPY_AND_ASSIGN(RasterImplementation);
};

}  // namespace raster
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_RASTER_IMPLEMENTATION_H_
