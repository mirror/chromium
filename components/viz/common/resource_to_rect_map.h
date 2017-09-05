// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCE_TO_RECT_MAP_H_
#define COMPONENTS_VIZ_COMMON_RESOURCE_TO_RECT_MAP_H_

#include <map>

#include "components/viz/common/resources/resource_id.h"
#include "ui/gfx/geometry/rect_f.h"

namespace viz {
using ResourceToRectMap = std::map<ResourceId, gfx::RectF>;
}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCE_TO_RECT_MAP_H_
