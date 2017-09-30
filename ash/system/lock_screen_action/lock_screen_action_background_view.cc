// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/lock_screen_action/lock_screen_action_background_view.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/i18n/rtl.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_state.h"
#include "ui/views/animation/square_ink_drop_ripple.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

class LockScreenActionBackgroundView::NoteBackground
    : public views::InkDropHostView {
 public:
  explicit NoteBackground(views::InkDropObserver* observer)
      : observer_(observer) {
    SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);
  }

  ~NoteBackground() override = default;

  std::unique_ptr<views::InkDrop> CreateInkDrop() override {
    std::unique_ptr<views::InkDropImpl> ink_drop =
        CreateDefaultFloodFillInkDropImpl();
    ink_drop->AddObserver(observer_);
    return std::move(ink_drop);
  }

  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override {
    gfx::Point center = base::i18n::IsRTL() ? GetLocalBounds().origin()
                                            : GetLocalBounds().top_right();
    return std::make_unique<views::FloodFillInkDropRipple>(
        size(), gfx::Insets(), center, SK_ColorBLACK, 1);
  }

  SkColor GetInkDropBaseColor() const override { return SK_ColorBLACK; }

 private:
  views::InkDropObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(NoteBackground);
};

LockScreenActionBackgroundView::TestApi::TestApi(
    LockScreenActionBackgroundView* action_background_view)
    : action_background_view_(action_background_view) {}

LockScreenActionBackgroundView::TestApi::~TestApi() = default;

views::View* LockScreenActionBackgroundView::TestApi::GetBackground() {
  return action_background_view_->background_;
}

LockScreenActionBackgroundView::LockScreenActionBackgroundView(
    base::OnceClosure on_delete)
    : on_delete_(std::move(on_delete)) {
  auto* layout_manager = new views::BoxLayout(views::BoxLayout::kVertical);
  layout_manager->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_STRETCH);
  SetLayoutManager(layout_manager);

  background_ = new NoteBackground(this);
  AddChildView(background_);
  layout_manager->SetFlexForView(background_, 1);
}

LockScreenActionBackgroundView::~LockScreenActionBackgroundView() {
  std::move(on_delete_).Run();
}

void LockScreenActionBackgroundView::AnimateShow(base::OnceClosure callback) {
  animation_end_callback_ = std::move(callback);
  animating_to_state_ = views::InkDropState::ACTIVATED;

  background_->AnimateInkDrop(views::InkDropState::ACTIVATED, nullptr);
}

void LockScreenActionBackgroundView::AnimateHide(base::OnceClosure callback) {
  animation_end_callback_ = std::move(callback);
  animating_to_state_ = views::InkDropState::HIDDEN;

  background_->AnimateInkDrop(views::InkDropState::HIDDEN, nullptr);
}

void LockScreenActionBackgroundView::InkDropAnimationStarted() {}

void LockScreenActionBackgroundView::InkDropAnimationEnded(
    views::InkDropState state) {
  if (animation_end_callback_.is_null() || state != animating_to_state_)
    return;

  std::move(animation_end_callback_).Run();
}

bool LockScreenActionBackgroundView::CanResize() const {
  return true;
}

bool LockScreenActionBackgroundView::CanMaximize() const {
  return true;
}

bool LockScreenActionBackgroundView::CanActivate() const {
  return false;
}

}  // namespace ash
