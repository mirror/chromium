// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_ANIMATED_ROUNDED_IMAGE_VIEW_H_
#define ASH_LOGIN_UI_ANIMATED_ROUNDED_IMAGE_VIEW_H_

#include <vector>

#include "ash/animation_frame.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"

namespace ash {

// A custom image view with rounded edges.
class AnimatedRoundedImageView : public views::View {
 public:
  // Constructs a new rounded image view with rounded corners of radius
  // |corner_radius|.
  explicit AnimatedRoundedImageView(int corner_radius);
  ~AnimatedRoundedImageView() override;

  // Show an animation.
  void SetAnimation(const Animation& animation, const gfx::Size& size);

  // Show a static image.
  void SetImage(const gfx::ImageSkia& image, const gfx::Size& size);

  // Start animation.
  void StartAnimation();
  // Stop any in-progress animation.
  void StopAnimation();

  // Overridden from views::View.
  gfx::Size CalculatePreferredSize() const override;
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  void SetAnimationWithoutStarting(const Animation& animation,
                                   const gfx::Size& size);
  void UpdateAnimationFrame();

  int active_frame_ = 0;
  Animation frames_;
  gfx::Size image_size_;
  int corner_radius_;

  base::OneShotTimer update_frame_timer_;

  DISALLOW_COPY_AND_ASSIGN(AnimatedRoundedImageView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_ANIMATED_ROUNDED_IMAGE_VIEW_H_
