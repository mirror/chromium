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

const int kSlideInDurationMs = 300;
const int kSlideOutDurationMs = 150;

// Creates a Widget containing an ExitFullscreenIndicatorView.
std::unique_ptr<views::Widget> CreatePopupWidget(
    gfx::NativeView parent_view,
    ExitFullscreenIndicatorView* view) {
  // Initialize the popup.
  std::unique_ptr<views::Widget> popup(new views::Widget);
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

ExitFullscreenIndicator::ExitFullscreenIndicator(gfx::NativeView parent_view) {
  animation_ = base::MakeUnique<gfx::SlideAnimation>(this);

  indicator_view_ = new ExitFullscreenIndicatorView();
  popup_ = CreatePopupWidget(parent_view, indicator_view_);
}

ExitFullscreenIndicator::~ExitFullscreenIndicator() {}

void ExitFullscreenIndicator::Show(const gfx::Rect& parent_bounds_in_screen) {
  parent_bounds_in_screen_ = parent_bounds_in_screen;
  animation_->SetSlideDuration(kSlideInDurationMs);
  animation_->Show();
}

void ExitFullscreenIndicator::Hide(bool animated) {
  if (!animated) {
    animation_->Reset(0);
    popup_->SetOpacity(0.f);
    popup_->Hide();
    return;
  }

  animation_->SetSlideDuration(kSlideOutDurationMs);
  animation_->Hide();
}

void ExitFullscreenIndicator::AnimationProgressed(
    const gfx::Animation* animation) {
  float opacity = static_cast<float>(
      animation_->CurrentValueBetween(kInitialOpacity, kFinalOpacity));
  if (opacity == kInitialOpacity) {
    popup_->Hide();
    return;
  }
  popup_->SetOpacity(opacity);
  int initial_offset = -indicator_view_->GetPreferredSize().height();
  int current_y = parent_bounds_in_screen_.y() +
                  animation_->CurrentValueBetween(initial_offset, kFinalOffset);
  gfx::Point origin(parent_bounds_in_screen_.CenterPoint().x() -
                        indicator_view_->GetPreferredSize().width() / 2,
                    current_y);
  popup_->SetBounds({origin, indicator_view_->GetPreferredSize()});
  popup_->Show();
}

void ExitFullscreenIndicator::AnimationEnded(const gfx::Animation* animation) {
  AnimationProgressed(animation);
}
