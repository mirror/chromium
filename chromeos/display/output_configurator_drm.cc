// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/display/output_configurator_drm.h"

#include "base/chromeos/chromeos_version.h"
#include "base/logging.h"
#include "base/message_pump_evdev.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "dbus/bus.h"
#include "dbus/exported_object.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/aura/root_window_host_drm.h"
#include "ui/base/output_drm.h"

namespace chromeos {

OutputConfiguratorDRM::OutputConfiguratorDRM()  {
  if (base::chromeos::IsRunningOnChromeOS()) {
    // Send the signal to powerd to tell it that we will take over output
    // control.
    // Note that this can be removed once the legacy powerd support is removed.
    chromeos::DBusThreadManager* manager = chromeos::DBusThreadManager::Get();
    dbus::Bus* bus = manager->GetSystemBus();
    dbus::ExportedObject* remote_object = bus->GetExportedObject(
        dbus::ObjectPath(power_manager::kPowerManagerServicePath));
    dbus::Signal signal(power_manager::kPowerManagerInterface,
                        power_manager::kUseNewMonitorConfigSignal);
    CHECK(signal.raw_message() != NULL);
    remote_object->SendSignal(&signal);

    int fd = aura::RootWindowHostDRM::GetDRMFd();
    // Cache the initial output state.
    if (!TryRecacheOutputs(fd)) {
      // Make sure the system has outputs at all (connected or not).
      std::vector<ui::OutputDRM*> all_outputs;
      CHECK(ui::DRMGetNumOutputs(fd) > 0);
    }
    State current_state = InferCurrentState(fd);
    if (current_state == STATE_INVALID) {
      // Unknown state.  Transition into the default state.
      State state = GetDefaultState();
      UpdateCacheAndOutputToState(fd, state);
    } else {
      // This is a valid state so just save it to |output_state_|.
      output_state_ = current_state;
    }
    CheckIsProjectingAndNotify();
  }
}

OutputConfiguratorDRM::~OutputConfiguratorDRM() {
}

void OutputConfiguratorDRM::UpdateCacheAndOutputToState(const DisplayInfo& info,
                                                        State new_state) {
  // TODO(sque)
  NOTIMPLEMENTED();
}

bool OutputConfiguratorDRM::ScreenPowerSet(bool power_on, bool all_displays) {
  // TODO(sque)
  NOTIMPLEMENTED();
  return true;
}

bool OutputConfiguratorDRM::SetDisplayMode(State new_state) {
  if (output_state_ == STATE_INVALID ||
      output_state_ == STATE_HEADLESS ||
      output_state_ == STATE_SINGLE)
    return false;

  UpdateCacheAndOutputToState(aura::RootWindowHostDRM::GetDRMFd(), new_state);
  return true;
}

bool OutputConfiguratorDRM::Dispatch(const base::NativeEvent& event) {
  // TODO(sque)
  NOTIMPLEMENTED();
  return true;
}

bool OutputConfiguratorDRM::TryRecacheOutputs(const DisplayInfo& info) {
  // TODO(sque)
  NOTIMPLEMENTED();
  return false;
}

bool OutputConfiguratorDRM::RecacheAndUseDefaultState() {
  if (!TryRecacheOutputs(aura::RootWindowHostDRM::GetDRMFd()))
    return false;
  State state = GetDefaultState();
  UpdateCacheAndOutputToState(aura::RootWindowHostDRM::GetDRMFd(), state);
  return true;
}

State OutputConfiguratorDRM::InferCurrentState(const DisplayInfo& info) const {
  // TODO(sque)
  NOTIMPLEMENTED();
  return STATE_SINGLE;
}

bool OutputConfiguratorDRM::IsProjecting() const {
  // TODO(sque)
  NOTIMPLEMENTED();
  return false;
}

}  // namespace chromeos
