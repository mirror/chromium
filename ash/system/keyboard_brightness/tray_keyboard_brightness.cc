// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/keyboard_brightness/tray_keyboard_brightness.h"

#include <algorithm>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/shell_observer.h"
#include "ash/shell_port.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/keyboard_brightness_control_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "ash/wm/maximize_mode/maximize_mode_controller.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/display/display.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace ash {
namespace tray {

namespace {

// A slider that ignores inputs
class ReadOnlySlider : public views::Slider {
 public:
  ReadOnlySlider() : Slider(nullptr) {}

 private:
  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override { return false; }
  bool OnMouseDragged(const ui::MouseEvent& event) override { return false; }
  void OnMouseReleased(const ui::MouseEvent& event) override {}
  bool OnKeyPressed(const ui::KeyEvent& event) override { return false; }

  // ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override {}
};

}  // namespace

class KeyboardBrightnessView : public ShellObserver,
                               public views::View {
 public:
  KeyboardBrightnessView(bool default_view, double initial_percent);
  ~KeyboardBrightnessView() override;

  bool is_default_view() const { return is_default_view_; }

  // |percent| is in the range [0.0, 100.0].
  void SetKeyboardBrightnessPercent(double percent);

  // ShellObserver:
  void OnMaximizeModeStarted() override;
  void OnMaximizeModeEnded() override;

 private:
  // views::View:
  void OnBoundsChanged(const gfx::Rect& old_bounds) override;

  ReadOnlySlider* slider_;

  // True if this view is for the default tray view. Used to control hide/show
  // behaviour of the default view when entering or leaving Maximize Mode.
  bool is_default_view_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardBrightnessView);
};

KeyboardBrightnessView::KeyboardBrightnessView(bool default_view,
                                               double initial_percent)
    : is_default_view_(default_view) {
  SetLayoutManager(new views::FillLayout);
  // Use CreateMultiTargetRowView() instead of CreateDefaultRowView() because
  // that's what the audio row uses and we want the two rows to layout with the
  // same insets.
  TriView* tri_view = TrayPopupUtils::CreateMultiTargetRowView();
  AddChildView(tri_view);

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  views::ImageView* icon = TrayPopupUtils::CreateMainImageView();
  icon->SetImage(
      gfx::CreateVectorIcon(kSystemMenuKeyboardBrightnessIcon, kMenuIconColor));
  tri_view->AddView(TriView::Container::START, icon);

  slider_ = new ReadOnlySlider();
  slider_->SetBorder(views::CreateEmptyBorder(gfx::Insets(0, 16)));
  slider_->SetValue(static_cast<float>(initial_percent / 100.0));
  slider_->SetAccessibleName(
      rb.GetLocalizedString(IDS_ASH_STATUS_TRAY_KEYBOARD_BRIGHTNESS));
  tri_view->AddView(TriView::Container::CENTER, slider_);

  if (is_default_view_) {
    Shell::Get()->AddShellObserver(this);
    SetVisible(Shell::Get()
                   ->maximize_mode_controller()
                   ->IsMaximizeModeWindowManagerEnabled());
  }
  tri_view->SetContainerVisible(TriView::Container::END, false);
}

KeyboardBrightnessView::~KeyboardBrightnessView() {
  if (is_default_view_)
    Shell::Get()->RemoveShellObserver(this);
}

void KeyboardBrightnessView::SetKeyboardBrightnessPercent(double percent) {
  slider_->SetValue(static_cast<float>(percent / 100.0));
}

void KeyboardBrightnessView::OnMaximizeModeStarted() {
  SetVisible(true);
}

void KeyboardBrightnessView::OnMaximizeModeEnded() {
  SetVisible(false);
}

void KeyboardBrightnessView::OnBoundsChanged(const gfx::Rect& old_bounds) {
  int w = width() - slider_->x();
  slider_->SetSize(gfx::Size(w, slider_->height()));
}

}  // namespace tray

TrayKeyboardBrightness::TrayKeyboardBrightness(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_DISPLAY_BRIGHTNESS),
      brightness_view_(nullptr),
      current_percent_(100.0),
      weak_ptr_factory_(this) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
}

TrayKeyboardBrightness::~TrayKeyboardBrightness() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

views::View* TrayKeyboardBrightness::CreateDefaultView(LoginStatus status) {
  CHECK(brightness_view_ == nullptr);
  brightness_view_ = new tray::KeyboardBrightnessView(true, current_percent_);
  return brightness_view_;
}

views::View* TrayKeyboardBrightness::CreateDetailedView(LoginStatus status) {
  CHECK(brightness_view_ == nullptr);
  brightness_view_ = new tray::KeyboardBrightnessView(false, current_percent_);
  return brightness_view_;
}

void TrayKeyboardBrightness::OnDefaultViewDestroyed() {
  if (brightness_view_ && brightness_view_->is_default_view())
    brightness_view_ = nullptr;
}

void TrayKeyboardBrightness::OnDetailedViewDestroyed() {
  if (brightness_view_ && !brightness_view_->is_default_view())
    brightness_view_ = nullptr;
}

void TrayKeyboardBrightness::UpdateAfterLoginStatusChange(LoginStatus status) {}

bool TrayKeyboardBrightness::ShouldShowShelf() const {
  return false;
}

void TrayKeyboardBrightness::KeyboardBrightnessChanged(int level,
                                                       bool user_initiated) {
  double percent = static_cast<double>(level);
  HandleKeyboardBrightnessChanged(percent, user_initiated);
}

void TrayKeyboardBrightness::HandleKeyboardBrightnessChanged(
    double percent, bool user_initiated) {
  current_percent_ = percent;

  if (brightness_view_)
    brightness_view_->SetKeyboardBrightnessPercent(percent);

  if (!user_initiated)
    return;

  if (brightness_view_ && brightness_view_->visible())
    SetDetailedViewCloseDelay(kTrayPopupAutoCloseDelayInSeconds);
  else
    ShowDetailedView(kTrayPopupAutoCloseDelayInSeconds, false);
}

}  // namespace ash
