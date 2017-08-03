// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/chromeos/midi_manager_cros.h"

#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/midis_client.h"
#include "dbus/object_path.h"
#include "media/midi/midi_manager_alsa.h"
#include "media/midi/midi_switches.h"

namespace {

// TODO(pmalani): Reference these from system_api once name is finalized.
constexpr char kMidisServiceName[] = "org.chromium.Midis";
constexpr char kMidisObjectPath[] = "/org/chromium/Midis";

}  // namespace

namespace midi {

MidiManagerCros::MidiManagerCros(MidiService* service) : MidiManager(service) {}

MidiManagerCros::~MidiManagerCros() = default;

// static
bool MidiManagerCros::SetupMojoChannel() {
  base::ScopedFD listen_fd =
      chromeos::DBusThreadManager::Get()
          ->GetMidisClient()
          ->BootstrapMojoConnection(kMidisServiceName,
                                    dbus::ObjectPath(kMidisObjectPath));

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
