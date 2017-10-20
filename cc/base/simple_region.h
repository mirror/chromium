// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_SIMPLE_REGION_H_
#define CC_BASE_SIMPLE_REGION_H_

#include "base/containers/stack_container.h"
#include "cc/base/base_export.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class InvalidationRegion;
class Region;

class CC_BASE_EXPORT SimpleRegion {
 public:
  SimpleRegion();
  ~SimpleRegion();

  SimpleRegion(SimpleRegion&& other);
  SimpleRegion& operator=(SimpleRegion&& other);

  bool EqualsForTesting(const SimpleRegion& other) const;
  bool EqualsForTesting(const Region& other) const;

  void Union(const gfx::Rect& rect);
  bool Intersects(const gfx::Rect& rect) const;
  void MergeInto(InvalidationRegion* region) const;

  const gfx::Rect& bounds() const { return bounds_; }
  bool empty() const { return rects_->size() == 0; }

 private:
  Region ToRegion() const;

  base::StackVector<gfx::Rect, 1> rects_;
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(SimpleRegion);
};

}  // namespace cc

#endif  // CC_BASE_SIMPLE_REGION_H_
