// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/scoped_ignore_process_events_checks.h"

#include "ash/shell.h"
#include "ui/aura/client/event_client.h"
#include "ui/aura/window.h"

namespace ash {

ScopedIgnoreProcessEventsChecks::ScopedIgnoreProcessEventsChecks() {
  for (auto* window : Shell::Get()->GetAllRootWindows()) {
    aura::client::EventClient* client = aura::client::GetEventClient(window);
    if (client)
      client->SetIgnoreProcessEventsChecks(true);
  }
}

ScopedIgnoreProcessEventsChecks::~ScopedIgnoreProcessEventsChecks() {
  for (auto* window : Shell::Get()->GetAllRootWindows()) {
    aura::client::EventClient* client = aura::client::GetEventClient(window);
    if (client)
      client->SetIgnoreProcessEventsChecks(false);
  }
}

}  // namespace ash
