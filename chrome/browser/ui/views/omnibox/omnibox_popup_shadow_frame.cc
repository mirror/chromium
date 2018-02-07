// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_popup_shadow_frame.h"

#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/compositor/shadow_layer.h"
#include "ui/gfx/transform.h"
#include "ui/views/background.h"
#include "ui/views/painter.h"

namespace {

constexpr int kMaxWindowHeight = 500;
constexpr int kCornerRadius = 8;
constexpr int kElevation = 16;
constexpr int kAnimationDurationMs = 250;

// The minimum height of the frame. Animations start from this value.
constexpr int kMinHeight = 20;

// The clip must animate its bounds, whereas the shadow must animate a
// transform. This means that even though they use the same tweener, they map to
// different geometry. To avoid the clip region overtaking the shadow, make it
// lag behind slightly.
constexpr int kClipAnimationLagMs = 100;

}  // namespace

OmniboxPopupShadowFrame::OmniboxPopupShadowFrame(views::View* contents)
    : shadow_(gfx::ShadowDetails::Get(kElevation, kCornerRadius)),
      insets_(GetBorderInsets()),
      contents_(contents) {
  // Paint the contents to a layer, so it can be clipped. First destroy any
  // existing layer, since the ui::Compositor may change when switching to a
  // new Widget.
  contents_->DestroyLayer();
  contents_->SetPaintToLayer();
  contents_->layer()->SetFillsBoundsOpaquely(false);

  clip_view_ = new views::View();
  clip_view_->SetPaintToLayer(ui::LAYER_NOT_DRAWN);
  clip_view_->AddChildView(contents_);

  clip_view_->SetBounds(insets_.left() + content_offset_,
                        insets_.top() + content_offset_, 0, 0);
  clip_view_->layer()->SetMasksToBounds(true);

  // |background_host_| includes the background and shadow layer, which
  // animate together using a layer transform. Add a layer for that.
  background_host_ = new views::View();
  background_host_->SetPaintToLayer();
  background_host_->layer()->SetFillsBoundsOpaquely(false);

  background_ = new views::View();
  background_->SetBounds(insets_.left(), insets_.top(), 0, 0);
  background_->SetBackground(views::CreateBackgroundFromPainter(
      views::Painter::CreateSolidRoundRectPainter(SK_ColorWHITE,
                                                  kCornerRadius)));
  background_host_->AddChildView(background_);

  contents_shadow_ = std::make_unique<ui::Shadow>();
  contents_shadow_->SetRoundedCornerRadius(kCornerRadius);
  contents_shadow_->Init(kElevation);

  background_host_->layer()->Add(contents_shadow_->layer());

  // Add as sibling views. Note the background is not clipped.
  AddChildView(background_host_);

  // Animating the clip layer a view causes lots of calls to schedule paints
  // that are not necessary, even though it is LAYER_NOT_DRAWN.
  AddChildView(clip_view_);
}

OmniboxPopupShadowFrame::~OmniboxPopupShadowFrame() = default;

// static
int OmniboxPopupShadowFrame::GetWindowHeight() {
  return kMaxWindowHeight;
}

// static
gfx::Insets OmniboxPopupShadowFrame::GetBorderInsets() {
  return gfx::ShadowValue::GetBlurRegion(
             gfx::ShadowDetails::Get(kElevation, kCornerRadius).values) +
         gfx::Insets(kCornerRadius);
}

// static
int OmniboxPopupShadowFrame::GetContentOffset() {
  return std::ceil(kCornerRadius -
                   std::sqrt(kCornerRadius * kCornerRadius / 2.0));
}

void OmniboxPopupShadowFrame::SetTargetBounds(const gfx::Rect& window_bounds) {
  gfx::Rect content_bounds = gfx::Rect(window_bounds.size());
  background_host_->SetSize(content_bounds.size());
  content_bounds.Inset(insets_);
  contents_->layer()->SchedulePaint(gfx::Rect(content_bounds.size()));

  const gfx::Rect old_shadow_bounds = contents_shadow_->content_bounds();
  if (old_shadow_bounds == content_bounds)
    return;

  contents_shadow_->SetContentBounds(content_bounds);
  background_->SetBoundsRect(content_bounds);

  // TODO(tapted): Currently this animates the background for any size change.
  // However, animating an increase in size can give the perception of lag. All
  // size reductions should animate, but only the very first grow animation
  // (sequence) should animate. But the target bounds can change often in the
  // first 250ms so figuring out when to stop animating is tricky.

  // Animate the background (rounded rect and shadow) with a transform.
  float scale_x = 1.0 * old_shadow_bounds.width() / content_bounds.width();
  float scale_y = 1.0 * std::max(old_shadow_bounds.height(), kMinHeight) /
                  content_bounds.height();

  // Start at the current transform, which might not be the identity matrix
  // if an animation is in progress.
  gfx::Transform transform = background_host_->layer()->transform();
  transform.Translate(-insets_.left(), insets_.top());
  // Animate from a height of at least one row.
  transform.Scale(scale_x, scale_y);
  transform.Translate(insets_.left(), -insets_.top());
  background_host_->layer()->SetTransform(transform);
  ui::ScopedLayerAnimationSettings transition(
      background_host_->layer()->GetAnimator());
  transition.SetTransitionDuration(
      base::TimeDelta::FromMilliseconds(kAnimationDurationMs));
  transition.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN_2);
  // Always animate towards the identity matrix.
  background_host_->layer()->SetTransform(gfx::Transform());

  // Inset the clip to avoid poking outside the rounded corners.
  content_bounds.Inset(content_offset_, content_offset_);
  const gfx::Rect target_bounds = content_bounds;
  // Always animate from the current height.
  content_bounds.set_height(clip_view_->layer()->bounds().height());
  clip_view_->SetBoundsRect(target_bounds);

  // If the height is being reduced, there's never a need to animate the clip.
  if (target_bounds.height() < content_bounds.height())
    return;

  // Restore the old content bounds an animate to the new one.
  clip_view_->layer()->SetBounds(content_bounds);
  ui::ScopedLayerAnimationSettings clip_transition(
      clip_view_->layer()->GetAnimator());
  clip_transition.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
      kAnimationDurationMs + kClipAnimationLagMs));
  clip_transition.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN_2);
  clip_view_->layer()->SetBounds(target_bounds);
}

const char* OmniboxPopupShadowFrame::GetClassName() const {
  return "OmniboxPopupShadowFrame";
}

void OmniboxPopupShadowFrame::Layout() {
  gfx::Rect rect = GetLocalBounds();
  background_->SetBoundsRect(rect);
  rect.Inset(insets_);
  clip_view_->SetBounds(content_offset_, content_offset_,
                        rect.width() - 2 * content_offset_, kMinHeight);
  // The contents is clipped, so it can just overdraw.
  contents_->SetBounds(0, 0, rect.width() - 2 * content_offset_,
                       kMaxWindowHeight);

  // Set a dummy contents size to seed the first animation.
  rect.set_height(kMinHeight);
  contents_shadow_->SetContentBounds(rect);
}
