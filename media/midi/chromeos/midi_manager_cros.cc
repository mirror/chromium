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
  // Here we should actually have a check to see that the mojo channel
  // has been set up.
  bool ret = SetupMojoChannel();
}

bool MidiManagerCros::SetupMojoChannel() {
  base::ScopedFD listen_fd =
      chromeos::DBusThreadManager::Get()
          ->GetMidisClient()
          ->BootstrapMojoConnection(std::string("org.chromium.Midis"),
                                    dbus::ObjectPath("/org/chromium/Midis"));

  if (!listen_fd.is_valid()) {
    return false;
  }

  // TODO(pmalani): Call code to boostrap the mojo IPC channel here.
  return true;
}

MidiManager* MidiManager::Create(MidiService* service) {
  // Note: Because of crbug.com/719489, chrome://flags does not affect
  // base::FeatureList::IsEnabled when you build target_os="chromeos" on Linux.
  if (base::FeatureList::IsEnabled(features::kMidiManagerCros))
    return new MidiManagerCros(service);
  return new MidiManagerAlsa(service);
}

}  // namespace midi
