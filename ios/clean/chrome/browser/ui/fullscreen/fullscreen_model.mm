// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_model.h"

#include <algorithm>

#include "base/logging.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenModel::FullscreenModel(FullscreenController* controller)
    : controller_(controller) {
  DCHECK(controller_);
}

bool FullscreenModel::IsToolbarFullyVisible() const {
  return CGFloatEquals(progress(), 1.0);
}

bool FullscreenModel::IsToolbarFullyHidden() const {
  return CGFloatEquals(progress(), 0.0);
}

void FullscreenModel::IncrementDisabledCounter() {
  if (++disabled_counter_ == 1U)
    controller_->OnFullscreenDisabled();
}

void FullscreenModel::DecrementDisabledCounter() {
  DCHECK_GT(disabled_counter_, 0U);
  if (!--disabled_counter_)
    controller_->OnFullscreenEnabled();
}

void FullscreenModel::ResetForNavigation() {
  SetProgress(1.0);
  scrolling_ = false;
  base_offset_ = NAN;
}

void FullscreenModel::AnimationEndedWithProgress(CGFloat progress) {
  DCHECK_GE(progress, 0.0);
  DCHECK_LE(progress, 1.0);
  progress_ = progress;
  if (!locked_)
    UpdateBaseOffset();
}

void FullscreenModel::SetToolbarHeight(CGFloat toolbar_height) {
  DCHECK_GE(toolbar_height, 0.0);
  toolbar_height_ = toolbar_height;
  SetProgress(1.0);
}

void FullscreenModel::SetYContentOffset(CGFloat y_content_offset) {
  y_content_offset_ = y_content_offset;

  if (std::isnan(base_offset_))
    UpdateBaseOffset();

  if (scrolling_) {
    CGFloat delta = base_offset_ - y_content_offset_;
    SetProgress(1.0 + delta / toolbar_height_);
  } else if (!locked_) {
    // If the base offset is not locked, update it for programmatic scrolls.
    base_offset_ = y_content_offset;
  }
}

void FullscreenModel::SetScrollViewIsScrolling(bool scrolling) {
  if (scrolling_ == scrolling)
    return;
  scrolling_ = scrolling;
  if (!scrolling_)
    controller_->OnScrollEventEnded();
}

void FullscreenModel::SetScrollViewIsDragging(bool dragging) {
  if (dragging_ == dragging)
    return;
  dragging_ = dragging;
  // Update the base offset when dragging occurs.
  if (dragging_ && !locked_)
    UpdateBaseOffset();
}

void FullscreenModel::SetBaseOffsetLocked(bool locked) {
  locked_ = locked;
}

void FullscreenModel::SetProgress(CGFloat progress) {
  progress = std::min(static_cast<CGFloat>(1.0), progress);
  progress = std::max(static_cast<CGFloat>(0.0), progress);
  if (CGFloatEquals(progress_, progress))
    return;
  progress_ = progress;
  controller_->OnProgressUpdated();
}

void FullscreenModel::UpdateBaseOffset() {
  base_offset_ = y_content_offset_ - (1.0 - progress_) * toolbar_height_;
}
