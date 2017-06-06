// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/region_map_utils.h"

namespace cc {

namespace region_map_utils {

Region UnionOfRegions(const RegionMap& region_map) {
  Region region;
  for (RegionMap::const_iterator it = region_map.begin();
       it != region_map.end(); ++it) {
    region.Union(it->second);
  }
  return region;
}

}  // namespace region_map_utils
}  // namespace cc
