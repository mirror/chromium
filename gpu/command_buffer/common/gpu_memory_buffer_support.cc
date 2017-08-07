// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2extchromium.h>

#include "base/logging.h"
#include "gpu/command_buffer/common/capabilities.h"

namespace gpu {

unsigned BufferFormatToInternalFormat(gfx::BufferFormat format) {
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
      return GL_RGB;
    case gfx::BufferFormat::RGBA_4444:
      return GL_RGBA;
    case gfx::BufferFormat::RGBX_8888:
      return GL_RGB;
    case gfx::BufferFormat::RGBA_8888:
      return GL_RGBA;
    case gfx::BufferFormat::BGRX_8888:
      return GL_RGB;
    case gfx::BufferFormat::BGRA_8888:
      return GL_BGRA_EXT;
    case gfx::BufferFormat::RGBA_F16:
      return GL_RGBA;
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

bool IsImageFormatCompatibleWithGpuMemoryBufferFormat(
    unsigned internalformat,
    gfx::BufferFormat format) {
  return BufferFormatToInternalFormat(format) == internalformat;
}

bool IsImageFromGpuMemoryBufferFormatSupported(
    gfx::BufferFormat format,
    const gpu::Capabilities& capabilities) {
  switch (format) {
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
      return capabilities.texture_format_atc;
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:
      return capabilities.texture_format_bgra8888;
    case gfx::BufferFormat::DXT1:
      return capabilities.texture_format_dxt1;
    case gfx::BufferFormat::DXT5:
      return capabilities.texture_format_dxt5;
    case gfx::BufferFormat::ETC1:
      return capabilities.texture_format_etc1;
    case gfx::BufferFormat::R_16:
      return capabilities.texture_norm16;
    case gfx::BufferFormat::R_8:
    case gfx::BufferFormat::RG_88:
      return capabilities.texture_rg;
    case gfx::BufferFormat::UYVY_422:
      return capabilities.image_ycbcr_422;
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::YVU_420:
      return true;
    case gfx::BufferFormat::RGBA_F16:
      return capabilities.texture_half_float_linear;
    case gfx::BufferFormat::YUV_420_BIPLANAR:
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
      // TODO(dcastagna): Determine ycbcr_420v_image on CrOS at runtime
      // querying minigbm. crbug.com/646148
      return true;
#else
      return capabilities.image_ycbcr_420v;
#endif
  }

  NOTREACHED();
  return false;
}

bool IsImageSizeValidForGpuMemoryBufferFormat(const gfx::Size& size,
                                              gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
      // Compressed images must have a width and height that's evenly divisible
      // by the block size.
      return size.width() % 4 == 0 && size.height() % 4 == 0;
    case gfx::BufferFormat::R_8:
    case gfx::BufferFormat::R_16:
    case gfx::BufferFormat::RG_88:
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::RGBA_F16:
      return true;
    case gfx::BufferFormat::YVU_420:
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      // U and V planes are subsampled by a factor of 2.
      return size.width() % 2 == 0 && size.height() % 2 == 0;
    case gfx::BufferFormat::UYVY_422:
      return size.width() % 2 == 0;
  }

  NOTREACHED();
  return false;
}

}  // namespace gpu
