// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_popup.h"

#include "base/bind.h"
#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_view.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/widget/widget.h"

namespace {

// Offsets with respect to the top y coordinate of the parent widget.
const int kFinalOffset = 45;

const float kInitialOpacity = 0.1f;
const float kFinalOpacity = 1.f;

const int kSlideInDurationMs = 300;
const int kSlideOutDurationMs = 150;

// Creates a Widget containing an FullscreenControlView.
std::unique_ptr<views::Widget> CreatePopupWidget(gfx::NativeView parent_view,
                                                 FullscreenControlView* view) {
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

FullscreenControlPopup::FullscreenControlPopup(
    gfx::NativeView parent_view,
    const base::RepeatingClosure& on_button_pressed)
    : control_view_(new FullscreenControlView(on_button_pressed)) {
  animation_ = base::MakeUnique<gfx::SlideAnimation>(this);
  animation_->Reset(0);
  popup_ = CreatePopupWidget(parent_view, control_view_);
}

FullscreenControlPopup::~FullscreenControlPopup() {}

void FullscreenControlPopup::Show(const gfx::Rect& parent_bounds_in_screen) {
  parent_bounds_in_screen_ = parent_bounds_in_screen;

  animation_->SetSlideDuration(kSlideInDurationMs);
  animation_->Show();

  // The default animation progress is 0. Call it once here then show the popup
  // to prevent potential flickering.
  AnimationProgressed(animation_.get());
  popup_->Show();
}

void FullscreenControlPopup::Hide(bool animated) {
  if (!animated) {
    animation_->Reset(0);
    popup_->SetOpacity(0.f);
    popup_->Hide();
    return;
  }

  animation_->SetSlideDuration(kSlideOutDurationMs);
  animation_->Hide();
}

void FullscreenControlPopup::AnimationProgressed(
    const gfx::Animation* animation) {
  float opacity = static_cast<float>(
      animation_->CurrentValueBetween(kInitialOpacity, kFinalOpacity));
  popup_->SetOpacity(opacity);
  int initial_offset = -control_view_->GetPreferredSize().height();
  int current_y = parent_bounds_in_screen_.y() +
                  animation_->CurrentValueBetween(initial_offset, kFinalOffset);
  gfx::Point origin(parent_bounds_in_screen_.CenterPoint().x() -
                        control_view_->GetPreferredSize().width() / 2,
                    current_y);
  popup_->SetBounds({origin, control_view_->GetPreferredSize()});
}

void FullscreenControlPopup::AnimationEnded(const gfx::Animation* animation) {
  if (animation_->GetCurrentValue() == 0.0) {
    // It's the end of the reversed animation. Just hide the popup in this case.
    popup_->Hide();
    return;
  }
  AnimationProgressed(animation);
}
