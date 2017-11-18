// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/raster_implementation.h"

#include "base/logging.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/common/capabilities.h"

namespace gpu {

namespace {

// TODO(vmiura): Fix layering violation: gpu can't include viz.

unsigned int TextureStorageFormat(viz::ResourceFormat format) {
  switch (format) {
    case viz::RGBA_8888:
      return GL_RGBA8_OES;
    case viz::BGRA_8888:
      return GL_BGRA8_EXT;
    case viz::RGBA_F16:
      return GL_RGBA16F_EXT;
    case viz::RGBA_4444:
      return GL_RGBA4;
    case viz::ALPHA_8:
      return GL_ALPHA8_EXT;
    case viz::LUMINANCE_8:
      return GL_LUMINANCE8_EXT;
    case viz::RGB_565:
      return GL_RGB565;
    case viz::RED_8:
      return GL_R8_EXT;
    case viz::LUMINANCE_F16:
      return GL_LUMINANCE16F_EXT;
    case viz::R16_EXT:
      return GL_R16_EXT;
    case viz::ETC1:
      break;
  }
  NOTREACHED();
  return GL_RGBA8_OES;
}

unsigned int GLDataFormat(viz::ResourceFormat format) {
  DCHECK_LE(format, viz::RESOURCE_FORMAT_MAX);
  static const GLenum format_gl_data_format[] = {
      GL_RGBA,           // RGBA_8888
      GL_RGBA,           // RGBA_4444
      GL_BGRA_EXT,       // BGRA_8888
      GL_ALPHA,          // ALPHA_8
      GL_LUMINANCE,      // LUMINANCE_8
      GL_RGB,            // RGB_565
      GL_ETC1_RGB8_OES,  // ETC1
      GL_RED_EXT,        // RED_8
      GL_LUMINANCE,      // LUMINANCE_F16
      GL_RGBA,           // RGBA_F16
      GL_RED_EXT,        // R16_EXT
  };
  static_assert(
      arraysize(format_gl_data_format) == (viz::RESOURCE_FORMAT_MAX + 1),
      "format_gl_data_format does not handle all cases.");
  return format_gl_data_format[format];
}

unsigned int GLDataType(viz::ResourceFormat format) {
  DCHECK_LE(format, viz::RESOURCE_FORMAT_MAX);
  static const GLenum format_gl_data_type[] = {
      GL_UNSIGNED_BYTE,           // RGBA_8888
      GL_UNSIGNED_SHORT_4_4_4_4,  // RGBA_4444
      GL_UNSIGNED_BYTE,           // BGRA_8888
      GL_UNSIGNED_BYTE,           // ALPHA_8
      GL_UNSIGNED_BYTE,           // LUMINANCE_8
      GL_UNSIGNED_SHORT_5_6_5,    // RGB_565,
      GL_UNSIGNED_BYTE,           // ETC1
      GL_UNSIGNED_BYTE,           // RED_8
      GL_HALF_FLOAT_OES,          // LUMINANCE_F16
      GL_HALF_FLOAT_OES,          // RGBA_F16
      GL_UNSIGNED_SHORT,          // R16_EXT
  };
  static_assert(
      arraysize(format_gl_data_type) == (viz::RESOURCE_FORMAT_MAX + 1),
      "format_gl_data_type does not handle all cases.");

  return format_gl_data_type[format];
}

unsigned int GLInternalFormat(viz::ResourceFormat format) {
  // In GLES2, the internal format must match the texture format. (It no longer
  // is true in GLES3, however it still holds for the BGRA extension.)
  // GL_EXT_texture_norm16 follows GLES3 semantics and only exposes a sized
  // internal format (GL_R16_EXT).
  if (format == viz::R16_EXT)
    return GL_R16_EXT;

  return GLDataFormat(format);
}

}  // namespace

RasterImplementation::RasterImplementation(gles2::GLES2Interface* gl,
                                           const gpu::Capabilities& caps)
    : gl_(gl),
      use_texture_storage_(caps.texture_storage),
      use_texture_storage_image_(caps.texture_storage_image) {}

RasterImplementation::~RasterImplementation() {}

void RasterImplementation::DeleteTextures(GLsizei n, const GLuint* textures) {
  gl_->DeleteTextures(n, textures);
};

void RasterImplementation::BindTexture(GLenum target, GLuint texture) {
  gl_->BindTexture(target, texture);
};

GLuint RasterImplementation::CreateAndConsumeTextureCHROMIUM(
    GLenum target,
    const GLbyte* mailbox) {
  return gl_->CreateAndConsumeTextureCHROMIUM(target, mailbox);
}

void RasterImplementation::TexStorageForRaster(GLenum target,
                                               viz::ResourceFormat format,
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

}  // namespace gpu
