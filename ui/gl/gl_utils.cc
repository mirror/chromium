// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains some useful utilities for the ui/gl classes.

#include "ui/gl/gl_utils.h"

#include "ui/gfx/color_space.h"
#include "ui/gl/gl_bindings.h"

namespace gl {

int GetGLColorSpace(const gfx::ColorSpace& color_space) {
  if (color_space == gfx::ColorSpace::CreateSCRGBLinear())
    return GL_COLOR_SPACE_SCRGB_LINEAR_CHROMIUM;
  return GL_COLOR_SPACE_UNSPECIFIED_CHROMIUM;
}

unsigned GLFormatForBufferFormat(gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::ATC:
      return GL_ATC_RGB_AMD;
    case gfx::BufferFormat::ATCIA:
      return GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
    case gfx::BufferFormat::DXT1:
      return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case gfx::BufferFormat::DXT5:
      return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case gfx::BufferFormat::ETC1:
      return GL_ETC1_RGB8_OES;
    case gfx::BufferFormat::R_8:
      return GL_RED_EXT;
    case gfx::BufferFormat::R_16:
      return GL_R16_EXT;
    case gfx::BufferFormat::RG_88:
      return GL_RG_EXT;
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::BGRX_1010102:
      return GL_RGB;
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBA_F16:
      return GL_RGBA;
    case gfx::BufferFormat::BGRA_8888:
      return GL_BGRA_EXT;
    case gfx::BufferFormat::YVU_420:
      return GL_RGB_YCRCB_420_CHROMIUM;
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      return GL_RGB_YCBCR_420V_CHROMIUM;
    case gfx::BufferFormat::UYVY_422:
      return GL_RGB_YCBCR_422_CHROMIUM;
  }

  NOTREACHED();
  return GL_RGBA;
}
}
