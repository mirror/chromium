// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_TARGET_H_
#define CC_ANIMATION_ANIMATION_TARGET_H_

#include "cc/animation/animation_export.h"
#include "cc/trees/element_id.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/transform.h"

namespace cc {

class Animation;
class FilterOperations;
class TransformOperations;

class CC_ANIMATION_EXPORT AnimationTarget {
 public:
  virtual ~AnimationTarget() {}
  virtual void NotifyClientOpacityAnimated(float opacity,
                                           Animation* animation) {}
  virtual void NotifyClientFilterAnimated(const FilterOperations& filter,
                                          Animation* animation) {}
  virtual void NotifyClientBoundsAnimated(const gfx::SizeF& size,
                                          Animation* animation) {}
  virtual void NotifyClientTransformOperationsAnimated(
      const TransformOperations& operations,
      Animation* animation) {}
  virtual void NotifyClientScrollOffsetAnimated(
      const gfx::ScrollOffset& scroll_offset,
      Animation* animation) {}
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_TARGET_H_
