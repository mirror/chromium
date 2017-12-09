// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/lock_screen.h"

#include <memory>
#include <utility>

#include "ash/keyboard/keyboard_observer_register.h"
#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/lock_debug_view.h"
#include "ash/login/ui/lock_window.h"
#include "ash/login/ui/login_constants.h"
#include "ash/login/ui/login_data_dispatcher.h"
#include "ash/public/interfaces/session_controller.mojom.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wallpaper/wallpaper_widget_controller.h"
#include "base/command_line.h"
#include "chromeos/chromeos_switches.h"
#include "ui/compositor/layer.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/keyboard/keyboard_util.h"

namespace ash {
namespace {

views::View* BuildContentsView(mojom::TrayActionState initial_note_action_state,
                               LoginDataDispatcher* data_dispatcher) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kShowLoginDevOverlay)) {
    return new LockDebugView(initial_note_action_state, data_dispatcher);
  }
  return new LockContentsView(initial_note_action_state, data_dispatcher);
}

ui::Layer* GetWallpaperLayerForWindow(aura::Window* window) {
  return RootWindowController::ForWindow(window)
      ->wallpaper_widget_controller()
      ->widget()
      ->GetLayer();
}

// Global lock screen instance. There can only ever be on lock screen at a
// time.
LockScreen* instance_ = nullptr;

}  // namespace

LockScreen::LockScreen(ScreenType type)
    : type_(type),
      tray_action_observer_(this),
      session_observer_(this),
      keyboard_observer_(this) {
  tray_action_observer_.Add(ash::Shell::Get()->tray_action());
  Shell::Get()->AddShellObserver(this);
}

LockScreen::~LockScreen() {
  Shell::Get()->RemoveShellObserver(this);
}

// static
LockScreen* LockScreen::Get() {
  CHECK(instance_);
  return instance_;
}

// static
void LockScreen::Show(ScreenType type) {
  CHECK(!instance_);
  instance_ = new LockScreen(type);

  // TODO(agawronska): Add tests for UI visibility when virtual keyboard
  // present. Disable virtual keyboard overscroll because it interferes with
  // scrolling login/lock content. See crbug.com/363635.
  keyboard::SetKeyboardOverscrollOverride(
      keyboard::KEYBOARD_OVERSCROLL_OVERRIDE_DISABLED);

  auto data_dispatcher = std::make_unique<LoginDataDispatcher>();
  auto* contents = BuildContentsView(
      ash::Shell::Get()->tray_action()->GetLockScreenNoteState(),
      data_dispatcher.get());

  auto* window = instance_->window_ = new LockWindow(Shell::GetAshConfig());
  window->SetBounds(display::Screen::GetScreen()->GetPrimaryDisplay().bounds());
  window->SetContentsView(contents);
  window->set_data_dispatcher(std::move(data_dispatcher));
  window->Show();
}

// static
bool LockScreen::IsShown() {
  return !!instance_;
}

void LockScreen::Destroy() {
  CHECK_EQ(instance_, this);

  // Restore the initial wallpaper bluriness if they were changed.
  for (auto it = initial_blur_.begin(); it != initial_blur_.end(); ++it)
    it->first->SetLayerBlur(it->second);
  window_->Close();
  delete instance_;
  instance_ = nullptr;

  keyboard::SetKeyboardOverscrollOverride(
      keyboard::KEYBOARD_OVERSCROLL_OVERRIDE_NONE);
}

void LockScreen::ToggleBlurForDebug() {
  // Save the initial wallpaper bluriness upon the first time this is called.
  if (instance_->initial_blur_.empty()) {
    for (aura::Window* window : Shell::GetAllRootWindows()) {
      ui::Layer* layer = GetWallpaperLayerForWindow(window);
      instance_->initial_blur_[layer] = layer->layer_blur();
    }
  }
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Layer* layer = GetWallpaperLayerForWindow(window);
    if (layer->layer_blur() > 0.0f) {
      layer->SetLayerBlur(0.0f);
    } else {
      layer->SetLayerBlur(login_constants::kBlurSigma);
    }
  }
}

LoginDataDispatcher* LockScreen::data_dispatcher() {
  return window_->data_dispatcher();
}

void LockScreen::OnLockScreenNoteStateChanged(mojom::TrayActionState state) {
  if (data_dispatcher())
    data_dispatcher()->SetLockScreenNoteState(state);
}

void LockScreen::OnSessionStateChanged(session_manager::SessionState state) {
  if (type_ == ScreenType::kLogin &&
      state == session_manager::SessionState::ACTIVE) {
    Destroy();
  }
}

void LockScreen::OnLockStateChanged(bool locked) {
  if (type_ != ScreenType::kLock)
    return;

  if (!locked)
    Destroy();
  Shell::Get()->metrics()->login_metrics_recorder()->Reset();
}

void LockScreen::OnVirtualKeyboardStateChanged(bool activated,
                                               aura::Window* root_window) {
  LOG(ERROR) << "[XXX] OnVirtualKeyboardStateChanged " << activated;
  UpdateKeyboardObserverFromStateChanged(
      activated, root_window, window_->GetNativeWindow()->GetRootWindow(),
      &keyboard_observer_);
}

void LockScreen::OnKeyboardBoundsChanging(const gfx::Rect& new_bounds) {
  LOG(ERROR) << "[XXX] OnKeyboardBoundsChanging";
}

void LockScreen::OnKeyboardClosed() {
  LOG(ERROR) << "[XXX] OnKeyboardClosed";
}

}  // namespace ash
