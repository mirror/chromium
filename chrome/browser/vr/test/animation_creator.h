// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_ANIMATION_CREATOR_H_
#define CHROME_BROWSER_VR_TEST_ANIMATION_CREATOR_H_

#include "cc/animation/animation.h"
#include "cc/animation/keyframed_animation_curve.h"

namespace vr {

// A convenience class for generating Animations
class AnimationCreator {
 public:
  AnimationCreator();

  std::unique_ptr<cc::Animation> transform_animation(int id, int group);
  std::unique_ptr<cc::Animation> size_animation(int id, int group);

  cc::TransformOperations& from_operations() { return from_operations_; }
  cc::TransformOperations& to_operations() { return to_operations_; }

  void set_from_size(const gfx::SizeF& size) { from_size_ = size; }
  void set_to_size(const gfx::SizeF& size) { to_size_ = size; }

  void set_time_offset(const base::TimeDelta& offset) { time_offset_ = offset; }

  static base::TimeTicks usToTicks(uint64_t us);
  static base::TimeDelta usToDelta(uint64_t us);

 private:
  base::TimeDelta begin_time_;
  base::TimeDelta end_time_;
  base::TimeDelta time_offset_;

  cc::TransformOperations from_operations_;
  cc::TransformOperations to_operations_;
  gfx::SizeF from_size_;
  gfx::SizeF to_size_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_ANIMATION_CREATOR_H_
