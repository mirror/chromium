// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_TARGET_H_
#define CC_ANIMATION_ANIMATION_TARGET_H_

#include "cc/animation/animation_export.h"
#include "third_party/skia/include/core/SkColor.h"

namespace gfx {
class ScrollOffset;
class SizeF;
}  // namespace gfx

namespace cc {

class Animation;
class FilterOperations;
class TransformOperations;

// A WorkletAnimationPlayer is an animations player that allows its animation
// timing to be controlled by an animator running in
// AnimationWorkletGlobalScope.
class CC_ANIMATION_EXPORT WorkletAnimationPlayer : public AnimationPlayer {
 public:
  virtual ~AnimationTarget() {}
  virtual void NotifyClientFloatAnimated(float opacity,
                                         int target_property_id,
                                         Animation* animation) {}
  virtual void NotifyClientFilterAnimated(const FilterOperations& filter,
                                          int target_property_id,
                                          Animation* animation) {}
  virtual void NotifyClientSizeAnimated(const gfx::SizeF& size,
                                        int target_property_id,
                                        Animation* animation) {}
  virtual void NotifyClientColorAnimated(SkColor color,
                                         int target_property_id,
                                         Animation* animation) {}
  virtual void NotifyClientBooleanAnimated(bool visibility,
                                           int target_property_id,
                                           Animation* animation) {}
  virtual void NotifyClientTransformOperationsAnimated(
      const TransformOperations& operations,
      int target_property_id,
      Animation* animation) {}
  virtual void NotifyClientScrollOffsetAnimated(
      const gfx::ScrollOffset& scroll_offset,
      int target_property_id,
      Animation* animation) {}
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_TARGET_H_
