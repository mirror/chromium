// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/back_button.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event_sink.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/widget/widget.h"

namespace ash {

BackButton::BackButton(ShelfView* shelf_view, Shelf* shelf)
    : views::ImageButton(nullptr), shelf_view_(shelf_view), shelf_(shelf) {
  SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);
  set_has_ink_drop_action_on_click(true);
  set_ink_drop_base_color(kShelfInkDropBaseColor);
  set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);

  SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ASH_SHELF_BACK_BUTTON_ACCESSIBLE_NAME));
  SetSize(gfx::Size(kShelfSize, kShelfSize));
  SetFocusPainter(TrayPopupUtils::CreateFocusPainter());
}

BackButton::~BackButton() = default;

gfx::Point BackButton::GetCenterPoint() const {
  // The button bounds could have a larger height than width (in the case of
  // touch-dragging the shelf upwards) or a larger width than height (in the
  // case of a shelf hide/show animation), so adjust the y-position of the
  // button's center to ensure correct layout.
  return gfx::Point(width() / 2.f, width() / 2.f);
}

void BackButton::OnBoundsAnimationFinished() {
  SchedulePaint();
}

void BackButton::OnGestureEvent(ui::GestureEvent* event) {
  ImageButton::OnGestureEvent(event);
  if (event->type() == ui::ET_GESTURE_TAP ||
      event->type() == ui::ET_GESTURE_TAP_DOWN) {
    GenerateAndSendBackEvent(*event);
  }
}

bool BackButton::OnMousePressed(const ui::MouseEvent& event) {
  ImageButton::OnMousePressed(event);
  GenerateAndSendBackEvent(event);
  return true;
}

void BackButton::OnMouseReleased(const ui::MouseEvent& event) {
  ImageButton::OnMouseReleased(event);
  GenerateAndSendBackEvent(event);
}

std::unique_ptr<views::InkDropRipple> BackButton::CreateInkDropRipple() const {
  const gfx::Point center = GetCenterPoint();
  const gfx::Rect bounds(center.x() - kAppListButtonRadius,
                         center.y() - kAppListButtonRadius,
                         2 * kAppListButtonRadius, 2 * kAppListButtonRadius);
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
  return std::make_unique<views::CircleInkDropMask>(size(), GetCenterPoint(),
                                                    kAppListButtonRadius);
}

void BackButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::ImageSkia img = CreateVectorIcon(kShelfBackIcon, SK_ColorWHITE);

  const bool is_tablet_mode = Shell::Get()
                                  ->tablet_mode_controller()
                                  ->IsTabletModeWindowManagerEnabled();
  // Sync the fade in/out of the back arrow with the movement of the shelf
  // items.
  const double current_animation_value =
      shelf_view_->GetAppListButtonAnimationCurrentValue();

  // Paint the back button in tablet mode and handle transition animations.
  int opacity = is_tablet_mode ? 255 : 0;
  if (shelf_->is_tablet_mode_animation_running()) {
    if (current_animation_value <= 0.0) {
      // The mode flipped but the animation hasn't begun, paint the old state.
      opacity = is_tablet_mode ? 0 : 255;
    } else {
      // Animate 0->255 into tablet mode, animate 255->0 into normal mode.
      opacity = static_cast<int>(current_animation_value * 255.0);
      opacity = is_tablet_mode ? opacity : (255 - opacity);
    }
  }

  canvas->DrawImageInt(img, bounds().width() / 2 - img.width() / 2,
                       bounds().height() / 2 - img.height() / 2, opacity);
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

  // Send the back event to the root window of the back button's widget.
  const views::Widget* widget = GetWidget();
  if (widget && widget->GetNativeWindow()) {
    aura::Window* root_window = widget->GetNativeWindow()->GetRootWindow();
    ui::KeyEvent key_event(event_type, ui::VKEY_BROWSER_BACK, ui::EF_NONE);
    ignore_result(
        root_window->GetHost()->event_sink()->OnEventFromSource(&key_event));
  }
}

}  // namespace ash
