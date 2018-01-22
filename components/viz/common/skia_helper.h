// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_SKIA_HELPER_H_
#define COMPONENTS_VIZ_COMMON_SKIA_HELPER_H_

#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/viz_common_export.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"

namespace viz {
VIZ_COMMON_EXPORT void PopulateSkBitmapWithResource(SkBitmap* sk_bitmap,
                                                    void* pixels,
                                                    const gfx::Size& size,
                                                    ResourceFormat format);

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_SKIA_HELPER_H_
