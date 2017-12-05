// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_RASTER_TEXTURE_FORMAT_H_
#define GPU_COMMAND_BUFFER_RASTER_TEXTURE_FORMAT_H_

namespace gpu {
namespace raster {

// If these values are modified, then it is likely that
// raster_texture_format_utils.cc has to be updated as well.
enum TextureFormat {
  RGBA_8888,
  RGBA_4444,
  BGRA_8888,
  ALPHA_8,
  LUMINANCE_8,
  RGB_565,
  ETC1,
  RED_8,
  LUMINANCE_F16,
  RGBA_F16,
  R16_EXT,
  TEXTURE_FORMAT_MAX = R16_EXT,
};

}  // namespace raster
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_RASTER_TEXTURE_FORMAT_H_
