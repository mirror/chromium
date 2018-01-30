// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_tooltip_manager.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/system/tray/tray_constants.h"
#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/aura/window.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/events/event.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/wm/core/window_animations.h"

namespace ash {
namespace {

// The time delay for showing the first tooltip, measured in msec.
constexpr int kTooltipAppearanceDelay = 1000;

// Tooltip layout constants.

// Shelf item tooltip height.
constexpr int kTooltipHeight = 24;

// The maximum width of the tooltip bubble.  Borrowed the value from
// ash/tooltip/tooltip_controller.cc
constexpr int kTooltipMaxWidth = 250;

// Shelf item tooltip internal text margins.
constexpr int kTooltipTopBottomMargin = 4;
constexpr int kTooltipLeftRightMargin = 8;

// The offset for the tooltip bubble - making sure that the bubble is spaced
// with a fixed gap. The gap is accounted for by the transparent arrow in the
// bubble and an additional 1px padding for the shelf item views.
constexpr int kArrowTopBottomOffset = 1;
constexpr int kArrowLeftRightOffset = 1;

// Tooltip's border interior thickness that defines its minimum height.
constexpr int kBorderInteriorThickness = kTooltipHeight / 2;

}  // namespace

// The implementation of tooltip of the launcher.
class ShelfTooltipManager::ShelfTooltipBubble
    : public views::BubbleDialogDelegateView {
 public:
  ShelfTooltipBubble(views::View* anchor,
                     views::BubbleBorder::Arrow arrow,
                     const base::string16& text,
                     ui::NativeTheme* theme,
                     bool asymmetrical_border)
      : views::BubbleDialogDelegateView(anchor, arrow) {
    set_close_on_deactivate(false);
    set_can_activate(false);
    set_accept_events(false);
    set_margins(gfx::Insets(kTooltipTopBottomMargin, kTooltipLeftRightMargin));
    set_shadow(views::BubbleBorder::NO_ASSETS);
    SetLayoutManager(std::make_unique<views::FillLayout>());
    views::Label* label = new views::Label(text);
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetEnabledColor(
        theme->GetSystemColor(ui::NativeTheme::kColorId_TooltipText));
    SkColor background_color =
        theme->GetSystemColor(ui::NativeTheme::kColorId_TooltipBackground);
    set_color(background_color);
    label->SetBackgroundColor(background_color);
    // The background is not opaque, so we can't do subpixel rendering.
    label->SetSubpixelRenderingEnabled(false);
    AddChildView(label);

    gfx::Insets insets(kArrowTopBottomOffset, kArrowLeftRightOffset);
    // Adjust the anchor location for asymmetrical borders of shelf item.
    if (asymmetrical_border && anchor->border())
      insets += anchor->border()->GetInsets();
    if (ui::MaterialDesignController::IsSecondaryUiMaterial())
      insets += gfx::Insets(-kBubblePaddingHorizontalBottom);
    set_anchor_view_insets(insets);

    // Place the bubble in the same display as the anchor.
    set_parent_window(
        anchor_widget()->GetNativeWindow()->GetRootWindow()->GetChildById(
            kShellWindowId_SettingBubbleContainer));

    views::BubbleDialogDelegateView::CreateBubble(this);
    if (!ui::MaterialDesignController::IsSecondaryUiMaterial()) {
      // These must both be called after CreateBubble.
      SetArrowPaintType(views::BubbleBorder::PAINT_TRANSPARENT);
      SetBorderInteriorThickness(kBorderInteriorThickness);
    }
  }

 private:
  // BubbleDialogDelegateView:
  gfx::Size CalculatePreferredSize() const override {
    const gfx::Size size = BubbleDialogDelegateView::CalculatePreferredSize();
    const int kTooltipMinHeight = kTooltipHeight - 2 * kTooltipTopBottomMargin;
    return gfx::Size(std::min(size.width(), kTooltipMaxWidth),
                     std::max(size.height(), kTooltipMinHeight));
  }

  int GetDialogButtons() const override { return ui::DIALOG_BUTTON_NONE; }

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipBubble);
};

ShelfTooltipManager::ShelfTooltipManager(Shelf* shelf)
    : timer_delay_(kTooltipAppearanceDelay),
      shelf_(shelf),
      weak_factory_(this) {}

ShelfTooltipManager::~ShelfTooltipManager() = default;

void ShelfTooltipManager::Close() {
  timer_.Stop();
  if (bubble_)
    bubble_->GetWidget()->Close();
  bubble_ = nullptr;
}

bool ShelfTooltipManager::IsVisible() const {
  return bubble_ && bubble_->GetWidget()->IsVisible();
}

void ShelfTooltipManager::ShowTooltipWithDelay(views::View* view) {
  if (ShouldShowTooltipForView(view)) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(timer_delay_),
                 base::Bind(&ShelfTooltipManager::ShowTooltip,
                            weak_factory_.GetWeakPtr(), view));
  }
}

void ShelfTooltipManager::ShowTooltipWithText(views::View* view,
                                              const base::string16& text,
                                              bool asymmetrical_border) {
  timer_.Stop();
  if (bubble_) {
    // Cancel the hiding animation to hide the old bubble immediately.
    ::wm::SetWindowVisibilityAnimationTransition(
        bubble_->GetWidget()->GetNativeWindow(), ::wm::ANIMATE_NONE);
    Close();
  }

  if (!ShouldShowTooltipForView(view))
    return;

  views::BubbleBorder::Arrow arrow = views::BubbleBorder::Arrow::NONE;
  switch (shelf_->alignment()) {
    case SHELF_ALIGNMENT_BOTTOM:
    case SHELF_ALIGNMENT_BOTTOM_LOCKED:
      arrow = views::BubbleBorder::BOTTOM_CENTER;
      break;
    case SHELF_ALIGNMENT_LEFT:
      arrow = views::BubbleBorder::LEFT_CENTER;
      break;
    case SHELF_ALIGNMENT_RIGHT:
      arrow = views::BubbleBorder::RIGHT_CENTER;
      break;
  }

  bubble_ = new ShelfTooltipBubble(view, arrow, text,
                                   shelf_->shelf_widget()->GetNativeTheme(),
                                   asymmetrical_border);
  aura::Window* window = bubble_->GetWidget()->GetNativeWindow();
  ::wm::SetWindowVisibilityAnimationType(
      window, ::wm::WINDOW_VISIBILITY_ANIMATION_TYPE_VERTICAL);
  ::wm::SetWindowVisibilityAnimationTransition(window, ::wm::ANIMATE_HIDE);
  bubble_->GetWidget()->Show();
}

void ShelfTooltipManager::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  // Close on any press events inside or outside the tooltip.
  if (event.type() == ui::ET_POINTER_DOWN)
    Close();
}

void ShelfTooltipManager::WillChangeVisibilityState(
    ShelfVisibilityState new_state) {
  if (new_state == SHELF_HIDDEN)
    Close();
}

void ShelfTooltipManager::OnAutoHideStateChanged(ShelfAutoHideState new_state) {
  if (new_state == SHELF_AUTO_HIDE_HIDDEN) {
    timer_.Stop();
    // AutoHide state change happens during an event filter, so immediate close
    // may cause a crash in the HandleMouseEvent() after the filter.  So we just
    // schedule the Close here.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&ShelfTooltipManager::Close, weak_factory_.GetWeakPtr()));
  }
}

}  // namespace ash
