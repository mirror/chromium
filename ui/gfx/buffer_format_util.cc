// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/buffer_format_util.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/numerics/safe_math.h"

namespace gfx {
namespace {

const BufferFormat kBufferFormats[] = {
    BufferFormat::ATC,       BufferFormat::ATCIA,
    BufferFormat::DXT1,      BufferFormat::DXT5,
    BufferFormat::ETC1,      BufferFormat::R_8,
    BufferFormat::R_16,      BufferFormat::RG_88,
    BufferFormat::BGR_565,   BufferFormat::RGBA_4444,
    BufferFormat::RGBX_8888, BufferFormat::RGBA_8888,
    BufferFormat::BGRX_8888, BufferFormat::BGRX_1010102,
    BufferFormat::BGRA_8888, BufferFormat::RGBA_F16,
    BufferFormat::UYVY_422,  BufferFormat::YUV_420_BIPLANAR,
    BufferFormat::YVU_420};

static_assert(arraysize(kBufferFormats) ==
                  (static_cast<int>(BufferFormat::LAST) + 1),
              "BufferFormat::LAST must be last value of kBufferFormats");


bool RowSizeForBufferFormatChecked(
    size_t width, BufferFormat format, size_t plane, size_t* size_in_bytes) {
  base::CheckedNumeric<size_t> checked_size = width;
  switch (format) {
    case BufferFormat::ATCIA:
    case BufferFormat::DXT5:
      DCHECK_EQ(0u, plane);
      *size_in_bytes = width;
      return true;
    case BufferFormat::ATC:
    case BufferFormat::DXT1:
    case BufferFormat::ETC1:
      DCHECK_EQ(0u, plane);
      DCHECK_EQ(0u, width % 2);
      *size_in_bytes = width / 2;
      return true;
    case BufferFormat::R_8:
      checked_size += 3;
      if (!checked_size.IsValid())
        return false;
      *size_in_bytes = (checked_size & ~0x3).ValueOrDie();
      return true;
    case BufferFormat::R_16:
    case BufferFormat::RG_88:
    case BufferFormat::BGR_565:
    case BufferFormat::RGBA_4444:
    case BufferFormat::UYVY_422:
      checked_size *= 2;
      checked_size += 3;
      if (!checked_size.IsValid())
        return false;
      *size_in_bytes = (checked_size & ~0x3).ValueOrDie();
      return true;
    case BufferFormat::BGRX_8888:
    case BufferFormat::BGRX_1010102:
    case BufferFormat::RGBX_8888:
    case BufferFormat::RGBA_8888:
    case BufferFormat::BGRA_8888:
      checked_size *= 4;
      if (!checked_size.IsValid())
        return false;
      *size_in_bytes = checked_size.ValueOrDie();
      return true;
    case BufferFormat::RGBA_F16:
      checked_size *= 8;
      if (!checked_size.IsValid())
        return false;
      *size_in_bytes = checked_size.ValueOrDie();
      return true;
    case BufferFormat::YVU_420:
      DCHECK_EQ(0u, width % 2);
      *size_in_bytes = width / SubsamplingFactorForBufferFormat(format, plane);
      return true;
    case BufferFormat::YUV_420_BIPLANAR:
      DCHECK_EQ(width % 2, 0u);
      *size_in_bytes = width;
      return true;
  }
  NOTREACHED();
  return false;
}

}  // namespace

std::vector<BufferFormat> GetBufferFormatsForTesting() {
  return std::vector<BufferFormat>(kBufferFormats,
                                   kBufferFormats + arraysize(kBufferFormats));
}

size_t NumberOfPlanesForBufferFormat(BufferFormat format) {
  switch (format) {
    case BufferFormat::ATC:
    case BufferFormat::ATCIA:
    case BufferFormat::DXT1:
    case BufferFormat::DXT5:
    case BufferFormat::ETC1:
    case BufferFormat::R_8:
    case BufferFormat::R_16:
    case BufferFormat::RG_88:
    case BufferFormat::BGR_565:
    case BufferFormat::RGBA_4444:
    case BufferFormat::RGBX_8888:
    case BufferFormat::RGBA_8888:
    case BufferFormat::BGRX_8888:
    case BufferFormat::BGRX_1010102:
    case BufferFormat::BGRA_8888:
    case BufferFormat::RGBA_F16:
    case BufferFormat::UYVY_422:
      return 1;
    case BufferFormat::YUV_420_BIPLANAR:
      return 2;
    case BufferFormat::YVU_420:
      return 3;
  }
  NOTREACHED();
  return 0;
}

size_t SubsamplingFactorForBufferFormat(BufferFormat format, size_t plane) {
  switch (format) {
    case BufferFormat::ATC:
    case BufferFormat::ATCIA:
    case BufferFormat::DXT1:
    case BufferFormat::DXT5:
    case BufferFormat::ETC1:
    case BufferFormat::R_8:
    case BufferFormat::R_16:
    case BufferFormat::RG_88:
    case BufferFormat::BGR_565:
    case BufferFormat::RGBA_4444:
    case BufferFormat::RGBX_8888:
    case BufferFormat::RGBA_8888:
    case BufferFormat::BGRX_8888:
    case BufferFormat::BGRX_1010102:
    case BufferFormat::BGRA_8888:
    case BufferFormat::RGBA_F16:
    case BufferFormat::UYVY_422:
      return 1;
    case BufferFormat::YVU_420: {
      static size_t factor[] = {1, 2, 2};
      DCHECK_LT(static_cast<size_t>(plane), arraysize(factor));
      return factor[plane];
    }
    case BufferFormat::YUV_420_BIPLANAR: {
      static size_t factor[] = {1, 2};
      DCHECK_LT(static_cast<size_t>(plane), arraysize(factor));
      return factor[plane];
    }
  }
  NOTREACHED();
  return 0;
}

size_t RowSizeForBufferFormat(size_t width, BufferFormat format, size_t plane) {
  size_t row_size = 0;
  bool valid = RowSizeForBufferFormatChecked(width, format, plane, &row_size);
  DCHECK(valid);
  return row_size;
}

size_t BufferSizeForBufferFormat(const Size& size, BufferFormat format) {
  size_t buffer_size = 0;
  bool valid = BufferSizeForBufferFormatChecked(size, format, &buffer_size);
  DCHECK(valid);
  return buffer_size;
}

bool BufferSizeForBufferFormatChecked(const Size& size,
                                      BufferFormat format,
                                      size_t* size_in_bytes) {
  base::CheckedNumeric<size_t> checked_size = 0;
  size_t num_planes = NumberOfPlanesForBufferFormat(format);
  for (size_t i = 0; i < num_planes; ++i) {
    size_t row_size = 0;
    if (!RowSizeForBufferFormatChecked(size.width(), format, i, &row_size))
      return false;
    base::CheckedNumeric<size_t> checked_plane_size = row_size;
    checked_plane_size *= size.height() /
                          SubsamplingFactorForBufferFormat(format, i);
    if (!checked_plane_size.IsValid())
      return false;
    checked_size += checked_plane_size.ValueOrDie();
    if (!checked_size.IsValid())
      return false;
  }
  *size_in_bytes = checked_size.ValueOrDie();
  return true;
}

size_t BufferOffsetForBufferFormat(const Size& size,
                                   BufferFormat format,
                                   size_t plane) {
  DCHECK_LT(plane, gfx::NumberOfPlanesForBufferFormat(format));
  switch (format) {
    case BufferFormat::ATC:
    case BufferFormat::ATCIA:
    case BufferFormat::DXT1:
    case BufferFormat::DXT5:
    case BufferFormat::ETC1:
    case BufferFormat::R_8:
    case BufferFormat::R_16:
    case BufferFormat::RG_88:
    case BufferFormat::BGR_565:
    case BufferFormat::RGBA_4444:
    case BufferFormat::RGBX_8888:
    case BufferFormat::RGBA_8888:
    case BufferFormat::BGRX_8888:
    case BufferFormat::BGRX_1010102:
    case BufferFormat::BGRA_8888:
    case BufferFormat::RGBA_F16:
    case BufferFormat::UYVY_422:
      return 0;
    case BufferFormat::YVU_420: {
      static size_t offset_in_2x2_sub_sampling_sizes[] = {0, 4, 5};
      DCHECK_LT(plane, arraysize(offset_in_2x2_sub_sampling_sizes));
      return offset_in_2x2_sub_sampling_sizes[plane] *
             (size.width() / 2 + size.height() / 2);
    }
    case gfx::BufferFormat::YUV_420_BIPLANAR: {
      static size_t offset_in_2x2_sub_sampling_sizes[] = {0, 4};
      DCHECK_LT(plane, arraysize(offset_in_2x2_sub_sampling_sizes));
      return offset_in_2x2_sub_sampling_sizes[plane] *
             (size.width() / 2 + size.height() / 2);
    }
  }
  NOTREACHED();
  return 0;
}

const char* BufferFormatToString(BufferFormat format) {
  switch (format) {
    case BufferFormat::ATC:
      return "ATC";
    case BufferFormat::ATCIA:
      return "ATCIA";
    case BufferFormat::DXT1:
      return "DXT1";
    case BufferFormat::DXT5:
      return "DXT5";
    case BufferFormat::ETC1:
      return "ETC1";
    case BufferFormat::R_8:
      return "R_8";
    case BufferFormat::R_16:
      return "R_16";
    case BufferFormat::RG_88:
      return "RG_88";
    case BufferFormat::BGR_565:
      return "BGR_565";
    case BufferFormat::RGBA_4444:
      return "RGBA_4444";
    case BufferFormat::RGBX_8888:
      return "RGBX_8888";
    case BufferFormat::RGBA_8888:
      return "RGBA_8888";
    case BufferFormat::BGRX_8888:
      return "BGRX_8888";
    case BufferFormat::BGRX_1010102:
      return "BGRX_1010102";
    case BufferFormat::BGRA_8888:
      return "BGRA_8888";
    case BufferFormat::RGBA_F16:
      return "RGBA_F16";
    case BufferFormat::YVU_420:
      return "YVU_420";
    case BufferFormat::YUV_420_BIPLANAR:
      return "YUV_420_BIPLANAR";
    case BufferFormat::UYVY_422:
      return "UYVY_422";
  }
  NOTREACHED()
      << "Invalid BufferFormat: "
      << static_cast<typename std::underlying_type<BufferFormat>::type>(format);
  return "Invalid Format";
}

#define GL_ATC_RGB_AMD                    0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD    0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

#define GL_ETC1_RGB8_OES                  0x8D64

#define GL_RED_EXT                        0x1903
#define GL_R16_EXT                        0x822A
#define GL_RG_EXT                         0x8227
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_BGRA_EXT                       0x80E1

#define GL_RGB_YCRCB_420_CHROMIUM 0x78FA
#define GL_RGB_YCBCR_422_CHROMIUM 0x78FB
#define GL_RGB_YCBCR_420V_CHROMIUM 0x78FC

#define GL_NONE                           0

uint32_t BufferFormatToGLFormat(BufferFormat format) {
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
    default:
      NOTREACHED();
      return GL_NONE;
  }
}

}  // namespace gfx
