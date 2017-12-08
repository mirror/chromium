// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/FragmentData.h"
#include "core/paint/PaintLayer.h"

namespace blink {

// These are defined here because of PaintLayer dependency.

FragmentData::RareData::RareData(const LayoutPoint& location_in_backing)
    : unique_id(NewUniqueObjectId()),
      location_in_backing(location_in_backing) {}

FragmentData::RareData::~RareData() {}

void FragmentData::SetLayer(std::unique_ptr<PaintLayer> layer) {
  if (rare_data_ || layer)
    EnsureRareData().layer = std::move(layer);
}

}  // namespace blink
