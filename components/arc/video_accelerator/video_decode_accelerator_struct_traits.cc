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

}  // namespace mojo
