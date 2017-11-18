// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_RASTER_INTERFACE_H_
#define GPU_COMMAND_BUFFER_CLIENT_RASTER_INTERFACE_H_

#include <GLES2/gl2.h>

#include "base/compiler_specific.h"
#include "components/viz/common/resources/resource_format.h"

namespace cc {
class DisplayItemList;
}  // namespace cc

extern "C" typedef struct _ClientBuffer* ClientBuffer;
extern "C" typedef struct _GLColorSpace* GLColorSpace;

namespace gpu {

enum RasterTexStorageFlags { kNone = 0, kOverlay = (1 << 0) };

class RasterInterface {
 public:
  RasterInterface() {}
  virtual ~RasterInterface() {}

  virtual void DeleteTextures(GLsizei n, const GLuint* textures) = 0;
  virtual void BindTexture(GLenum target, GLuint texture) = 0;

  virtual GLuint CreateAndConsumeTextureCHROMIUM(GLenum target,
                                                 const GLbyte* mailbox) = 0;

  virtual void TexStorageForRaster(GLenum target,
                                   viz::ResourceFormat format,
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

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_RASTER_INTERFACE_H_
