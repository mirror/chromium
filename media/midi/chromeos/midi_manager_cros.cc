// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/chromeos/midi_manager_cros.h"

#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/midis_client.h"
#include "dbus/object_path.h"
#include "media/midi/midi_manager_alsa.h"
#include "media/midi/midi_switches.h"

namespace midi {

// Helper function for debug logs
// TODO(pmalani): Delete before submission!
void pr(const char* fmt, ...) {
  static FILE* logfile = NULL;
  va_list vl;
  if (!logfile)
    logfile = fopen("/tmp/client_log.txt", "w");
  if (!logfile)
    exit(-1);
  va_start(vl, fmt);
  vfprintf(logfile, fmt, vl);
  fflush(logfile);
  va_end(vl);
}

MidiManagerCros::MidiManagerCros(MidiService* service) : MidiManager(service) {}

MidiManagerCros::~MidiManagerCros() = default;

void MidiManagerCros::StartInitialization() {
  pr("11111\n");
  chromeos::DBusThreadManager::Get()->GetMidisClient()->BootstrapMojoConnection(
      std::string("org.chromium.Midis"),
      dbus::ObjectPath("/org/chromium/Midis"));
  pr("2222\n");
}

MidiManager* MidiManager::Create(MidiService* service) {
  // Note: Because of crbug.com/719489, chrome://flags does not affect
  // base::FeatureList::IsEnabled when you build target_os="chromeos" on Linux.
  if (base::FeatureList::IsEnabled(features::kMidiManagerCros))
    return new MidiManagerCros(service);
  return new MidiManagerAlsa(service);
}

}  // namespace midi
