// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_REGION_MAP_UTILS_H_
#define CC_LAYERS_REGION_MAP_UTILS_H_

#include "base/containers/flat_map.h"
#include "cc/base/region.h"
#include "cc/cc_export.h"
#include "cc/input/touch_action.h"

namespace cc {

typedef base::flat_map<TouchAction, Region> RegionMap;

namespace region_map_utils {

// Unions all regions contained in RegionMap.
Region CC_EXPORT UnionOfRegions(const RegionMap& region_map);

}  // namespace region_map_utils
}  // namespace cc

#endif  // CC_LAYERS_REGION_MAP_UTILS_H_
