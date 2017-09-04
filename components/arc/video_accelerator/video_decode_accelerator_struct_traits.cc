// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/video_accelerator/video_decode_accelerator_struct_traits.h"

namespace mojo {

bool StructTraits<arc::mojom::CropRectDataView, gfx::Rect>::Read(
    arc::mojom::CropRectDataView data,
    gfx::Rect* out) {
  out->set_x(data.left());
  out->set_y(data.top());
  out->set_width(data.right() - out->x());
  out->set_height(data.bottom() - out->y());
  return true;
}

bool StructTraits<arc::mojom::PictureBufferFormatDataView,
                  arc::PictureBufferFormat>::
    Read(arc::mojom::PictureBufferFormatDataView data,
         arc::PictureBufferFormat* out) {
  out->pixel_format = data.pixel_format();
  out->buffer_size = data.buffer_size();
  out->min_num_buffers = data.min_num_buffers();
  out->coded_width = data.coded_width();
  out->coded_height = data.coded_height();
  return true;
}

}  // namespace mojo
