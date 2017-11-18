// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_RASTER_IMPLEMENTATION_H_
#define GPU_COMMAND_BUFFER_CLIENT_RASTER_IMPLEMENTATION_H_

#include "base/macros.h"
#include "gles2_impl_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_interface.h"

namespace gpu {

struct Capabilities;

class GLES2_IMPL_EXPORT RasterImplementation : public RasterInterface {
 public:
  RasterImplementation(gles2::GLES2Interface* gl,
                       const gpu::Capabilities& caps);
  ~RasterImplementation() override;

  void DeleteTextures(GLsizei n, const GLuint* textures) override;
  void BindTexture(GLenum target, GLuint texture) override;

  GLuint CreateAndConsumeTextureCHROMIUM(GLenum target,
                                         const GLbyte* mailbox) override;

  void TexStorageForRaster(GLenum target,
                           viz::ResourceFormat format,
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

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_RASTER_IMPLEMENTATION_H_
