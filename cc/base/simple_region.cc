// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/simple_region.h"

#include "cc/base/invalidation_region.h"

namespace cc {
const int kMaxRectCount = 256;

SimpleRegion::SimpleRegion() = default;

SimpleRegion::~SimpleRegion() = default;

SimpleRegion::SimpleRegion(SimpleRegion&& other) = default;

SimpleRegion& SimpleRegion::operator=(SimpleRegion&& other) = default;

bool SimpleRegion::EqualsForTesting(const SimpleRegion& other) const {
  return ToRegion() == other.ToRegion();
}

bool SimpleRegion::EqualsForTesting(const Region& other) const {
  return ToRegion() == other;
}

void SimpleRegion::Union(const gfx::Rect& rect) {
  if (rects_->size() == kMaxRectCount) {
    rects_->back().Union(rect);
    return;
  }
  rects_->push_back(rect);
  bounds_.Union(rect);
}

bool SimpleRegion::Intersects(const gfx::Rect& rect) const {
  for (const auto& r : *(rects_.operator->())) {
    if (r.Intersects(rect))
      return true;
  }
  return false;
}

void SimpleRegion::MergeInto(InvalidationRegion* region) const {
  for (const auto& r : *(rects_.operator->()))
    region->Union(r);
}

Region SimpleRegion::ToRegion() const {
  Region region;
  for (const auto& r : *(rects_.operator->()))
    region.Union(r);

  return region;
}

}  // namespace cc
