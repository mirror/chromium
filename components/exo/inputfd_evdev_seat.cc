// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/inputfd_evdev_seat.h"

#include "base/memory/ptr_util.h"
#include "components/exo/inputfd_evdev_device.h"
#include "components/exo/inputfd_evdev_seat_delegate.h"
#include "components/exo/shell_surface.h"
#include "components/exo/surface.h"
#include "ui/events/ozone/gamepad/gamepad_provider_ozone.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// InputfdEvdevSeat, public:

InputfdEvdevSeat::InputfdEvdevSeat(InputfdEvdevSeatDelegate* delegate)
    : delegate_(delegate) {
  auto* helper = WMHelper::GetInstance();
  helper->AddFocusObserver(this);
  OnWindowFocused(helper->GetFocusedWindow(), nullptr);
}

InputfdEvdevSeat::~InputfdEvdevSeat() {
  if (focused_)
    ui::GamepadProviderOzone::GetInstance()->RemoveGamepadObserver(this);
  delegate_->OnInputfdEvdevSeatDestroying(this);
  WMHelper::GetInstance()->RemoveFocusObserver(this);
  DispatchFocusOut();
}

////////////////////////////////////////////////////////////////////////////////
// WMHelper::FocusObserver overrides:

void InputfdEvdevSeat::OnWindowFocused(aura::Window* gained_focus,
                                       aura::Window* lost_focus) {
  Surface* target = nullptr;
  if (gained_focus) {
    target = Surface::AsSurface(gained_focus);
    if (!target) {
      aura::Window* top_level_window = gained_focus->GetToplevelWindow();
      if (top_level_window)
        target = ShellSurface::GetMainSurface(top_level_window);
    }
  }

  bool focused = target && delegate_->CanAcceptGamepadEventsForSurface(target);
  if (focused_ != focused) {
    focused_ = focused;
    if (focused) {
      ui::GamepadProviderOzone::GetInstance()->AddGamepadObserver(this);
      OnGamepadDevicesUpdated();
      DispatchFocusIn(target);
    } else {
      ui::GamepadProviderOzone::GetInstance()->RemoveGamepadObserver(this);
      DispatchFocusOut();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// ui::GamepadObserver overrides:

void InputfdEvdevSeat::OnGamepadDevicesUpdated() {
  std::vector<ui::InputDevice> gamepad_devices =
      ui::GamepadProviderOzone::GetInstance()->GetGamepadDevices();

  base::flat_map<int, std::unique_ptr<InputfdEvdevDevice>> new_gamepads;

  // Copy the "still connected gamepads".
  for (auto& device : gamepad_devices) {
    auto it = gamepads_.find(device.id);
    if (it != gamepads_.end()) {
      new_gamepads[device.id] = std::move(it->second);
      gamepads_.erase(it);
    }
  }

  // Remove each disconected gamepad.
  gamepads_.clear();

  // Add each new connected gamepad.
  for (auto& device : gamepad_devices) {
    if (new_gamepads.find(device.id) == new_gamepads.end())
      new_gamepads[device.id] = base::MakeUnique<InputfdEvdevDevice>(
          device, delegate_->DeviceAdded());
  }

  new_gamepads.swap(gamepads_);
}

////////////////////////////////////////////////////////////////////////////////
// InputfdEvdevSeat, private:

void InputfdEvdevSeat::DispatchFocusIn(Surface* surface) {
  for (auto& device : gamepads_) {
    device.second->OnFocusIn(surface);
  }
}

void InputfdEvdevSeat::DispatchFocusOut() {
  for (auto& device : gamepads_) {
    device.second->OnFocusOut();
  }
}

}  // namespace exo
