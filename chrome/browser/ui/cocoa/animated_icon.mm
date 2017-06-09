// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/animated_icon.h"

#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"

AnimatedIcon::AnimatedIcon(const gfx::VectorIcon& icon, NSView* host_view)
    : icon_(icon),
      host_view_(host_view),
      duration_(gfx::GetDurationOfAnimation(icon)),
      animation_(base::MakeUnique<gfx::LinearAnimation>(this)) {
  animation_->SetDuration(duration_.InMilliseconds());
}

AnimatedIcon::~AnimatedIcon() {}

void AnimatedIcon::Animate() {
  animation_->Start();
}

void AnimatedIcon::PaintIcon(gfx::Canvas* canvas, NSRect frame) {
  base::TimeDelta elapsed = animation_->GetCurrentValue() * duration_;
  int size = GetDefaultSizeOfVectorIcon(icon_);
  canvas->Translate(
      gfx::Vector2d((NSWidth(frame) - size) / 2, (NSHeight(frame) - size) / 2));
  gfx::PaintVectorIcon(canvas, icon_, color_, elapsed);
}

void AnimatedIcon::AnimationEnded(const gfx::Animation* animation) {}

void AnimatedIcon::AnimationProgressed(const gfx::Animation* animation) {
  [host_view_ setNeedsDisplay:YES];
}
