// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "raster_texture_format_utils.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include "base/logging.h"
#include "base/macros.h"

namespace gpu {
namespace raster {

unsigned int TextureStorageFormat(TextureFormat format) {
  switch (format) {
    case RGBA_8888:
      return GL_RGBA8_OES;
    case BGRA_8888:
      return GL_BGRA8_EXT;
    case RGBA_F16:
      return GL_RGBA16F_EXT;
    case RGBA_4444:
      return GL_RGBA4;
    case ALPHA_8:
      return GL_ALPHA8_EXT;
    case LUMINANCE_8:
      return GL_LUMINANCE8_EXT;
    case RGB_565:
      return GL_RGB565;
    case RED_8:
      return GL_R8_EXT;
    case LUMINANCE_F16:
      return GL_LUMINANCE16F_EXT;
    case R16_EXT:
      return GL_R16_EXT;
    case ETC1:
      break;
  }
  NOTREACHED();
  return GL_RGBA8_OES;
}

unsigned int GLDataFormat(TextureFormat format) {
  DCHECK_LE(format, TEXTURE_FORMAT_MAX);
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
  static_assert(arraysize(format_gl_data_format) == (TEXTURE_FORMAT_MAX + 1),
                "format_gl_data_format does not handle all cases.");
  return format_gl_data_format[format];
}

unsigned int GLDataType(TextureFormat format) {
  DCHECK_LE(format, TEXTURE_FORMAT_MAX);
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
  static_assert(arraysize(format_gl_data_type) == (TEXTURE_FORMAT_MAX + 1),
                "format_gl_data_type does not handle all cases.");

  return format_gl_data_type[format];
}

unsigned int GLInternalFormat(TextureFormat format) {
  // In GLES2, the internal format must match the texture format. (It no longer
  // is true in GLES3, however it still holds for the BGRA extension.)
  // GL_EXT_texture_norm16 follows GLES3 semantics and only exposes a sized
  // internal format (GL_R16_EXT).
  if (format == R16_EXT)
    return GL_R16_EXT;

  return GLDataFormat(format);
}

}  // namespace raster
}  // namespace gpu
