// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/animated_rounded_image_view.h"

#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skia_util.h"

namespace ash {

AnimatedRoundedImageView::AnimatedRoundedImageView(int corner_radius)
    : corner_radius_(corner_radius) {}

AnimatedRoundedImageView::~AnimatedRoundedImageView() {}

void AnimatedRoundedImageView::SetAnimation(const Animation& animation,
                                            const gfx::Size& size) {
  SetAnimationWithoutStarting(animation, size);
  StartAnimation();
}

void AnimatedRoundedImageView::SetImage(const gfx::ImageSkia& image,
                                        const gfx::Size& size) {
  AnimationFrame frame;
  frame.image = image;
  SetAnimationWithoutStarting({frame}, size);
  StopAnimation();
}

void AnimatedRoundedImageView::StartAnimation() {
  active_frame_ = -1;
  UpdateAnimationFrame();
}

void AnimatedRoundedImageView::StopAnimation() {
  active_frame_ = 0;
  update_frame_timer_.Stop();
  SchedulePaint();
}

gfx::Size AnimatedRoundedImageView::CalculatePreferredSize() const {
  return gfx::Size(image_size_.width() + GetInsets().width(),
                   image_size_.height() + GetInsets().height());
}

void AnimatedRoundedImageView::OnPaint(gfx::Canvas* canvas) {
  if (frames_.empty())
    return;

  View::OnPaint(canvas);
  gfx::Rect image_bounds(size());
  image_bounds.ClampToCenteredSize(GetPreferredSize());
  image_bounds.Inset(GetInsets());
  const SkScalar kRadius[8] = {
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_),
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_),
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_),
      SkIntToScalar(corner_radius_), SkIntToScalar(corner_radius_)};
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(image_bounds), kRadius);
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  canvas->DrawImageInPath(frames_[active_frame_].image, image_bounds.x(),
                          image_bounds.y(), path, flags);
}

void AnimatedRoundedImageView::SetAnimationWithoutStarting(
    const Animation& animation,
    const gfx::Size& size) {
  image_size_ = size;

  frames_.reserve(animation.size());
  for (AnimationFrame frame : animation) {
    // Try to get the best image quality for the animation.
    frame.image = gfx::ImageSkiaOperations::CreateResizedImage(
        frame.image, skia::ImageOperations::RESIZE_BEST, size);
    DCHECK(frame.image.bitmap()->isImmutable());
    frames_.push_back(frame);
  }
}

void AnimatedRoundedImageView::UpdateAnimationFrame() {
  // Note: |active_frame_| may be invalid.
  active_frame_ = (active_frame_ + 1) % frames_.size();
  SchedulePaint();

  // Schedule next frame update.
  update_frame_timer_.Start(
      FROM_HERE, frames_[active_frame_].duration,
      base::BindRepeating(&AnimatedRoundedImageView::UpdateAnimationFrame,
                          base::Unretained(this)));
}

}  // namespace ash
