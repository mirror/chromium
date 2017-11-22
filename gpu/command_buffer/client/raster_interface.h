// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_RASTER_INTERFACE_H_
#define GPU_COMMAND_BUFFER_CLIENT_RASTER_INTERFACE_H_

#include <GLES2/gl2.h>

#include "base/compiler_specific.h"
#include "gpu/command_buffer/common/raster_texture_format.h"

namespace cc {
class DisplayItemList;
}  // namespace cc

extern "C" typedef struct _ClientBuffer* ClientBuffer;
extern "C" typedef struct _GLColorSpace* GLColorSpace;

namespace gpu {
namespace raster {

enum RasterTexStorageFlags { kNone = 0, kOverlay = (1 << 0) };

class RasterInterface {
 public:
  RasterInterface() {}
  virtual ~RasterInterface() {}

  virtual GLuint64 InsertFenceSyncCHROMIUM() = 0;
  virtual void GenUnverifiedSyncTokenCHROMIUM(GLuint64 fence_sync,
                                              GLbyte* sync_token) = 0;
  virtual void VerifySyncTokensCHROMIUM(GLbyte** sync_tokens,
                                        GLsizei count) = 0;
  virtual void WaitSyncTokenCHROMIUM(const GLbyte* sync_token) = 0;
  virtual void OrderingBarrierCHROMIUM() = 0;
  virtual void Finish() = 0;
  virtual void ShallowFlushCHROMIUM() = 0;

  virtual GLenum GetGraphicsResetStatusKHR() = 0;
  virtual void GetIntegerv(GLenum pname, GLint* params) = 0;

  virtual void GenTextures(GLsizei n, GLuint* textures) = 0;
  virtual void DeleteTextures(GLsizei n, const GLuint* textures) = 0;
  virtual void BindTexture(GLenum target, GLuint texture) = 0;
  virtual void ActiveTexture(GLenum texture) = 0;

  virtual void GenerateMipmap(GLenum target) = 0;
  virtual void TexParameteri(GLenum target, GLenum pname, GLint param) = 0;

  virtual void GenMailboxCHROMIUM(GLbyte* mailbox) = 0;
  virtual void ProduceTextureDirectCHROMIUM(GLuint texture,
                                            GLenum target,
                                            const GLbyte* mailbox) = 0;
  virtual GLuint CreateAndConsumeTextureCHROMIUM(GLenum target,
                                                 const GLbyte* mailbox) = 0;

  virtual GLuint CreateImageCHROMIUM(ClientBuffer buffer,
                                     GLsizei width,
                                     GLsizei height,
                                     GLenum internalformat) = 0;
  virtual void BindTexImage2DCHROMIUM(GLenum target, GLint imageId) = 0;
  virtual void ReleaseTexImage2DCHROMIUM(GLenum target, GLint imageId) = 0;
  virtual void DestroyImageCHROMIUM(GLuint image_id) = 0;

  virtual void CompressedTexImage2D(GLenum target,
                                    GLint level,
                                    GLenum internalformat,
                                    GLsizei width,
                                    GLsizei height,
                                    GLint border,
                                    GLsizei imageSize,
                                    const void* data) = 0;
  virtual void TexSubImage2D(GLenum target,
                             GLint level,
                             GLint xoffset,
                             GLint yoffset,
                             GLsizei width,
                             GLsizei height,
                             GLenum format,
                             GLenum type,
                             const void* pixels) = 0;

  virtual void TexStorageForRaster(GLenum target,
                                   TextureFormat format,
                                   GLsizei width,
                                   GLsizei height,
                                   RasterTexStorageFlags flags) = 0;

  virtual void SetColorSpaceMetadataCHROMIUM(GLuint texture_id,
                                             GLColorSpace color_space) = 0;

  virtual void BeginRasterCHROMIUM(GLuint texture_id,
                                   GLuint sk_color,
                                   GLuint msaa_sample_count,
                                   GLboolean can_use_lcd_text,
                                   GLboolean use_distance_field_text,
                                   GLint pixel_config) = 0;
  virtual void RasterCHROMIUM(const cc::DisplayItemList* list,
                              GLint translate_x,
                              GLint translate_y,
                              GLint clip_x,
                              GLint clip_y,
                              GLint clip_w,
                              GLint clip_h,
                              GLfloat post_translate_x,
                              GLfloat post_translate_y,
                              GLfloat post_scale) = 0;
  virtual void EndRasterCHROMIUM() = 0;
};

}  // namespace raster
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_RASTER_INTERFACE_H_
