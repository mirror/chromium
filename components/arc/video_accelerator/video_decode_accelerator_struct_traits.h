// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_

#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "components/arc/video_accelerator/video_accelerator.h"
#include "ui/gfx/geometry/rect.h"

namespace mojo {

template <>
struct StructTraits<arc::mojom::PictureBufferFormatDataView,
                    arc::PictureBufferFormat> {
  static uint32_t pixel_format(const arc::PictureBufferFormat& p) {
    return p.pixel_format;
  }
  static uint32_t buffer_size(const arc::PictureBufferFormat& p) {
    return p.buffer_size;
  }
  static uint32_t min_num_buffers(const arc::PictureBufferFormat& p) {
    return p.min_num_buffers;
  }
  static uint32_t coded_width(const arc::PictureBufferFormat& p) {
    return p.coded_width;
  }
  static uint32_t coded_height(const arc::PictureBufferFormat& p) {
    return p.coded_height;
  }
  static bool Read(arc::mojom::PictureBufferFormatDataView data,
                   arc::PictureBufferFormat* out);
};

}  // namespace mojo

#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_
