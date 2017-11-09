// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/back_button.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/event_sink.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/widget/widget.h"

#include "ui/views/border.h"
#include "base/strings/utf_string_conversions.h"

namespace ash {

BackButton::BackButton(InkDropButtonListener* listener,
                       ShelfView* shelf_view,
                       Shelf* shelf)
    : views::ImageButton(nullptr), shelf_view_(shelf_view), shelf_(shelf) {
  LOG(ERROR) << listener;
  SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);
  set_has_ink_drop_action_on_click(true);
  set_ink_drop_base_color(kShelfInkDropBaseColor);
  set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);

  SetBorder(views::CreateSolidBorder(2, SK_ColorBLUE));
  SetAccessibleName(base::UTF8ToUTF16("hodor"));
}

BackButton::~BackButton() = default;

gfx::Point BackButton::GetBackButtonCenterPoint() const {
  const int x_mid = width() / 2.f;
  const int y_mid = height() / 2.f;

  const ShelfAlignment alignment = shelf_->alignment();
  if (alignment == SHELF_ALIGNMENT_BOTTOM ||
      alignment == SHELF_ALIGNMENT_BOTTOM_LOCKED) {
    return gfx::Point(x_mid, x_mid);
  } else if (alignment == SHELF_ALIGNMENT_RIGHT) {
    return gfx::Point(y_mid, y_mid);
  } else {
    DCHECK_EQ(alignment, SHELF_ALIGNMENT_LEFT);
    return gfx::Point(width() - y_mid, y_mid);
  }
}

void BackButton::OnGestureEvent(ui::GestureEvent* event) {}

bool BackButton::OnMousePressed(const ui::MouseEvent& event) {
  ImageButton::OnMousePressed(event);
  GenerateAndSendBackEvent(event);
  return true;
}

void BackButton::OnMouseReleased(const ui::MouseEvent& event) {
  ImageButton::OnMouseReleased(event);
  GenerateAndSendBackEvent(event);
}

void BackButton::GetAccessibleNodeData(ui::AXNodeData* node_data) {}

std::unique_ptr<views::InkDropRipple> BackButton::CreateInkDropRipple() const {
  gfx::Point center = GetBackButtonCenterPoint();
  gfx::Rect bounds(center.x() - kAppListButtonRadius,
                   center.y() - kAppListButtonRadius, 2 * kAppListButtonRadius,
                   2 * kAppListButtonRadius);
  return std::make_unique<views::FloodFillInkDropRipple>(
      size(), GetLocalBounds().InsetsFrom(bounds),
      GetInkDropCenterBasedOnLastEvent(), GetInkDropBaseColor(),
      ink_drop_visible_opacity());
}

std::unique_ptr<views::InkDrop> BackButton::CreateInkDrop() {
  std::unique_ptr<views::InkDropImpl> ink_drop =
      Button::CreateDefaultInkDropImpl();
  ink_drop->SetShowHighlightOnHover(false);
  return std::move(ink_drop);
}

std::unique_ptr<views::InkDropMask> BackButton::CreateInkDropMask() const {
  return std::make_unique<views::CircleInkDropMask>(
      size(), GetBackButtonCenterPoint(), kAppListButtonRadius);
}

void BackButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::ImageSkia back_button =
      CreateVectorIcon(kShelfBackIcon, SK_ColorTRANSPARENT);

  const bool is_tablet_mode = Shell::Get()
                                  ->tablet_mode_controller()
                                  ->IsTabletModeWindowManagerEnabled();
  const double current_animation_value =
      shelf_view_->GetAppListButtonAnimationCurrentValue();

  // Paint the back button in tablet mode and handle transition animations.
  int opacity = is_tablet_mode ? 255 : 0;
  if (shelf_view_->is_tablet_mode_animation_running()) {
    if (current_animation_value <= 0.0) {
      // The mode flipped but the animation hasn't begun, paint the old state.
      opacity = is_tablet_mode ? 0 : 255;
    } else {
      // Animate 0->255 into tablet mode, animate 255->0 into normal mode.
      opacity = static_cast<int>(current_animation_value * 255.0);
      opacity = is_tablet_mode ? opacity : (255 - opacity);
    }
  }

  canvas->DrawImageInt(
      back_button, bounds().width() / 2 - back_button.width() / 2,
      bounds().height() / 2 - back_button.height() / 2, opacity);
}

void BackButton::GenerateAndSendBackEvent(
    const ui::LocatedEvent& original_event) {
  ui::EventType event_type;
  switch (original_event.type()) {
    case ui::ET_MOUSE_PRESSED:
    case ui::ET_GESTURE_TAP_DOWN:
      event_type = ui::ET_KEY_PRESSED;
      break;
    case ui::ET_MOUSE_RELEASED:
    case ui::ET_GESTURE_TAP:
      event_type = ui::ET_KEY_RELEASED;
      base::RecordAction(base::UserMetricsAction("Tablet_BackButton"));
      break;
    default:
      return;
  }

  // Send the back event to the root window of the app list button's widget.
  const views::Widget* widget = GetWidget();
  if (widget && widget->GetNativeWindow()) {
    aura::Window* root_window = widget->GetNativeWindow()->GetRootWindow();
    ui::KeyEvent key_event(event_type, ui::VKEY_BROWSER_BACK, ui::EF_NONE);
    ignore_result(
        root_window->GetHost()->event_sink()->OnEventFromSource(&key_event));
  }
}

}  // namespace ash
