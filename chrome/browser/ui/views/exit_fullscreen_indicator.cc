// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/exit_fullscreen_indicator.h"

#include "base/bind.h"
#include "chrome/browser/ui/views/exit_fullscreen_indicator_view.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/widget/widget.h"

namespace {

// Offsets with respect to the top y coordinate of the parent widget.
const int kFinalOffset = 45;

const float kInitialOpacity = 0.1f;
const float kFinalOpacity = 1.f;

constexpr base::TimeDelta kSlideInDuration =
    base::TimeDelta::FromMilliseconds(300);
constexpr base::TimeDelta kSlideOutDuration =
    base::TimeDelta::FromMilliseconds(150);

// Creates a Widget containing an ExitFullscreenIndicatorView.
views::Widget* CreatePopupWidget(gfx::NativeView parent_view,
                                 ExitFullscreenIndicatorView* view) {
  // Initialize the popup.
  views::Widget* popup = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = parent_view;
  params.accept_events = false;
  popup->Init(params);
  popup->SetContentsView(view);

  return popup;
}

}  // namespace

ExitFullscreenIndicator::ExitFullscreenIndicator(views::Widget* parent_widget) {
  parent_widget_ = parent_widget;

  indicator_view_ = new ExitFullscreenIndicatorView();
  popup_.reset(
      CreatePopupWidget(parent_widget->GetNativeView(), indicator_view_));
}

ExitFullscreenIndicator::~ExitFullscreenIndicator() {}

void ExitFullscreenIndicator::Show() {
  popup_->Show();
  popup_->GetLayer()->SetOpacity(kInitialOpacity);
  int initialOffset = -indicator_view_->GetPreferredSize().height();
  popup_->SetBounds(CalculateBounds(initialOffset));

  ui::ScopedLayerAnimationSettings animation_(
      popup_->GetLayer()->GetAnimator());
  animation_.SetTransitionDuration(kSlideInDuration);
  animation_.SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

  popup_->GetLayer()->SetOpacity(kFinalOpacity);
  popup_->SetBounds(CalculateBounds(kFinalOffset));
}

void ExitFullscreenIndicator::Hide(bool animated) {
  popup_->GetLayer()->GetAnimator()->AbortAllAnimations();

  if (!animated) {
    popup_->Hide();
  }

  ui::ScopedLayerAnimationSettings animation_(
      popup_->GetLayer()->GetAnimator());
  animation_.SetTransitionDuration(kSlideOutDuration);
  animation_.SetTweenType(gfx::Tween::FAST_OUT_LINEAR_IN);
  animation_.AddObserver(this);

  popup_->GetLayer()->SetOpacity(kInitialOpacity);
  int initialOffset = -indicator_view_->GetPreferredSize().height();
  popup_->SetBounds(CalculateBounds(initialOffset));
}

void ExitFullscreenIndicator::OnImplicitAnimationsCompleted() {
  if (popup_.get()->GetLayer()->opacity() == kInitialOpacity) {
    popup_->Hide();
  }
}

gfx::Rect ExitFullscreenIndicator::CalculateBounds(int y_offset) const {
  gfx::Rect parent_bounds = parent_widget_->GetClientAreaBoundsInScreen();

  gfx::Point origin(parent_bounds.CenterPoint().x() -
                        indicator_view_->GetPreferredSize().width() / 2,
                    parent_bounds.y() + y_offset);
  return {origin, indicator_view_->GetPreferredSize()};
}
