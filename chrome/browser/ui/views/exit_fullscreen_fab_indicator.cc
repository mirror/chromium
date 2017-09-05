// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/exit_fullscreen_fab_indicator.h"

#include "base/bind.h"
#include "chrome/browser/ui/views/fab_icon_view.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/views/widget/widget.h"

namespace {

// Offsets with respect to the top y coordinate of the parent widget.
const int kFabInitialOffset = -48;
const int kFabFinalOffset = 45;

const int kSlideInDurationMs = 300;
const int kSlideOutDurationMs = 150;

}  // namespace

ExitFullscreenFabIndicator::ExitFullscreenFabIndicator(
    views::Widget* parent_widget) {
  animation_.reset(new gfx::SlideAnimation(this));
  animation_->SetTweenType(gfx::Tween::LINEAR_OUT_SLOW_IN);

  parent_widget_ = parent_widget;

  fab_view_ = new FabIconView();
  fab_view_->SetIcon(vector_icons::kIcCloseIcon);
  fab_view_->SizeToPreferredSize();
  popup_.reset(FabIconView::CreatePopupWidget(parent_widget->GetNativeView(),
                                              fab_view_, false));
}

ExitFullscreenFabIndicator::~ExitFullscreenFabIndicator() {}

void ExitFullscreenFabIndicator::Show() {
  animation_->SetSlideDuration(kSlideInDurationMs);
  animation_->Show();
}

void ExitFullscreenFabIndicator::Hide(bool animated) {
  if (!animated) {
    animation_->Reset(0);
    popup_->SetOpacity(0.f);
    popup_->Hide();
    return;
  }

  animation_->SetSlideDuration(kSlideOutDurationMs);
  animation_->Hide();
}

void ExitFullscreenFabIndicator::AnimationProgressed(
    const gfx::Animation* animation) {
  float opacity = static_cast<float>(animation_->CurrentValueBetween(0.1, 0.8));
  if (opacity == 0.1f) {
    popup_->Hide();
    return;
  }

  popup_->SetOpacity(opacity);
  popup_->Show();

  gfx::Rect parent_bounds = parent_widget_->GetClientAreaBoundsInScreen();

  int current_y = parent_bounds.y() + animation_->CurrentValueBetween(
                                          kFabInitialOffset, kFabFinalOffset);
  gfx::Point origin(parent_bounds.CenterPoint().x() -
                        fab_view_->GetPreferredSize().width() / 2,
                    current_y);
  popup_->SetBounds({origin, fab_view_->GetPreferredSize()});
}

void ExitFullscreenFabIndicator::AnimationEnded(
    const gfx::Animation* animation) {
  AnimationProgressed(animation);
}
