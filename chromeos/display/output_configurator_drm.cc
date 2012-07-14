// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/display/output_configurator_drm.h"

#include <xf86drmMode.h>

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

namespace aura {
class CompositorLock;
}  // namespace aura

namespace {

static bool FindMirrorModeForOutputs(const ui::OutputDRM& output_one,
                                     const ui::OutputDRM& output_two,
                                     ui::ModeDRM* mode_one,
                                     ui::ModeDRM* mode_two) {
  std::vector<ui::ModeDRM> one_modes;
  std::vector<ui::ModeDRM> two_modes;
  output_one.GetModes(&one_modes);
  output_two.GetModes(&two_modes);
  unsigned int one_index = 0;
  unsigned int two_index = 0;
  while ((one_index < one_modes.size()) && (two_index < two_modes.size())) {
    const ui::ModeDRM& one_mode = one_modes[one_index];
    const ui::ModeDRM& two_mode = two_modes[two_index];
    int one_width = one_mode.width();
    int one_height = one_mode.height();
    int two_width = two_mode.width();
    int two_height = two_mode.height();
    if ((one_mode.width() == two_mode.width()) &&
        (one_mode.height() == two_mode.height())) {
      *mode_one = one_mode;
      *mode_two = two_mode;
      return true;
    } else {
      // The sort order of the modes is NOT by mode area but is sorted by width,
      // then by height within each like width.
      if (one_width > two_width) {
        one_index += 1;
      } else if (one_width < two_width) {
        two_index += 1;
      } else {
        if (one_height > two_height) {
          one_index += 1;
        } else {
          two_index += 1;
        }
      }
    }
  }
  return false;
}

static void ConfigureCrtc(int fd,
                          const drmModeCrtc& crtc,
                          int x,
                          int y,
                          const ui::ModeDRM& mode,
                          ui::OutputDRM* output) {
#if 0
  aura::RootWindowHostDRM* window_host =
      aura::RootWindowHostDRM::Create(gfx::Rect(mode.width(), mode.height()));
  //uint32_t crtc_id = window_host->GetAcceleratedWidget();
  scoped_refptr<aura::CompositorLock> lock =
      window_host->GetRootWindow()->GetCompositorLock();

  drmModeCrtc new_crtc = crtc;
  new_crtc.x = x;
  new_crtc.y = y;
  new_crtc.mode = mode.mode_info;
  output->SetCRTC(new_crtc);

  window_host->SetSize();
#endif
}

static void GetOutputConfiguration(
    int fd, int connector_id, drmModeCrtc* crtc, ui::ModeDRM* mode) {
  ui::OutputDRM output(fd, connector_id);
  output.GetCRTC(crtc);
  *mode = output.GetPreferredMode();
}

}  // namespace

volatile void* ____foo = reinterpret_cast<void*>(GetOutputConfiguration);
volatile void* ____goo = reinterpret_cast<void*>(ConfigureCrtc);
volatile void* ____bar = reinterpret_cast<void*>(FindMirrorModeForOutputs);

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
