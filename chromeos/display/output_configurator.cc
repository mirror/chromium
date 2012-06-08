// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/display/output_configurator.h"

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/Xrandr.h>

// Xlib defines Status as int which causes our include of dbus/bus.h to fail
// when it tries to name an enum Status.  Thus, we need to undefine it (note
// that this will cause a problem if code needs to use the Status type).
// RootWindow causes similar problems in that there is a Chromium type with that
// name.
#undef Status
#undef RootWindow

#include "base/chromeos/chromeos_version.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/perftimer.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "dbus/bus.h"
#include "dbus/exported_object.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

#if defined(USE_EVDEV)
#include "base/message_pump_evdev.h"
#else
#include "base/message_pump_aurax11.h"
#endif

namespace chromeos {

OutputConfigurator::OutputConfigurator()
    : output_state_(STATE_INVALID),
      mirror_supported_(false),
      primary_output_index_(-1),
      secondary_output_index_(-1) {
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
  }
}

OutputConfigurator::~OutputConfigurator() {
}

bool OutputConfigurator::CycleDisplayMode() {
  VLOG(1) << "CycleDisplayMode";
  bool did_change = false;
  if (base::chromeos::IsRunningOnChromeOS()) {
    // Rules:
    // - if there are 0 or 1 displays, do nothing and return false.
    // - use y-coord of CRTCs to determine if we are mirror, primary-first, or
    // secondary-first.  The cycle order is:
    //   mirror->primary->secondary->mirror.
    State new_state = STATE_INVALID;
    switch (output_state_) {
      case STATE_DUAL_MIRROR:
        new_state = STATE_DUAL_PRIMARY_ONLY;
        break;
      case STATE_DUAL_PRIMARY_ONLY:
        new_state = STATE_DUAL_SECONDARY_ONLY;
        break;
      case STATE_DUAL_SECONDARY_ONLY:
        new_state = mirror_supported_ ?
            STATE_DUAL_MIRROR :
            STATE_DUAL_PRIMARY_ONLY;
        break;
      default:
        // Do nothing - we aren't in a mode which we can rotate.
        break;
    }
    if (STATE_INVALID != new_state)
      did_change = SetDisplayMode(new_state);
  }
  return did_change;
}

State OutputConfigurator::GetDefaultState() const {
  State state = STATE_HEADLESS;
  if (-1 != primary_output_index_) {
    if (-1 != secondary_output_index_)
      state = mirror_supported_ ? STATE_DUAL_MIRROR : STATE_DUAL_PRIMARY_ONLY;
    else
      state = STATE_SINGLE;
  }
  return state;
}

void OutputConfigurator::CheckIsProjectingAndNotify() {
  chromeos::DBusThreadManager* manager = chromeos::DBusThreadManager::Get();
  dbus::Bus* bus = manager->GetSystemBus();
  dbus::ObjectProxy* power_manager_proxy = bus->GetObjectProxy(
      power_manager::kPowerManagerServiceName,
      dbus::ObjectPath(power_manager::kPowerManagerServicePath));
  dbus::MethodCall method_call(
      power_manager::kPowerManagerInterface,
      power_manager::kSetIsProjectingMethod);
  dbus::MessageWriter writer(&method_call);
  writer.AppendBool(IsProjecting());
  power_manager_proxy->CallMethod(
      &method_call,
      dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      dbus::ObjectProxy::EmptyResponseCallback());
}

}  // namespace chromeos
