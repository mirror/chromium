// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_

#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "ui/gfx/geometry/rect.h"

namespace mojo {

template <>
struct StructTraits<arc::mojom::CropRectDataView, gfx::Rect> {
  static uint32_t left(const gfx::Rect& r) {
    DCHECK_GE(r.x(), 0);
    return r.x();
  }

  static uint32_t top(const gfx::Rect& r) {
    DCHECK_GE(r.y(), 0);
    return r.y();
  }

  static uint32_t right(const gfx::Rect& r) {
    DCHECK_GE(r.right(), r.x());
    return r.right();
  }
  static uint32_t bottom(const gfx::Rect& r) {
    DCHECK_GE(r.bottom(), r.y());
    return r.bottom();
  }

  static bool Read(arc::mojom::CropRectDataView data, gfx::Rect* out);
};

}  // namespace mojo

#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_
