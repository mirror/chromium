// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/host/ash_window_tree_host.h"

#include <memory>

#include "ash/host/ash_window_tree_host_init_params.h"
#include "ash/host/ash_window_tree_host_platform.h"
#include "ash/host/ash_window_tree_host_unified.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/manager/display_manager.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

AshWindowTreeHost::AshWindowTreeHost() {}

AshWindowTreeHost::~AshWindowTreeHost() = default;

void AshWindowTreeHost::TranslateLocatedEvent(ui::LocatedEvent* event) {
  // NOTE: This code is not called in mus/mash, it is handled on the server
  // side.
  // TODO(sky): remove this when mus is the default http://crbug.com/763996.
  DCHECK_EQ(aura::Env::Mode::LOCAL, aura::Env::GetInstance()->mode());

  if (event->IsTouchEvent())
    return;

  aura::WindowTreeHost* wth = AsWindowTreeHost();
  aura::Window* root_window = wth->window();
  aura::client::ScreenPositionClient* screen_position_client =
      aura::client::GetScreenPositionClient(root_window);
  gfx::Rect local(wth->GetBoundsInPixels().size());
  local.Inset(GetHostInsets());

  if (screen_position_client && !local.Contains(event->location())) {
    gfx::Point location;
    if (Shell::Get()->display_manager()->IsInUnifiedMode() &&
        event->HasNativeEvent()) {
      // The below happens only in Unified Mode on Linux ChromeOS build.
      // On X11 Ozone, explicit window capture is sent to the unified host stub
      // window rather than mirroring display host, which means explicit capture
      // doesn't work. As a result, the events are sent to the target mirroring
      // host (where the dragged winodw is currently located), rather than to
      // the source mirroring host (where the dragged window used to be). But
      // X11 is still doing its implicit grab, and the location of the sent
      // event is relative to the source host. The below makes it relative to
      // the target host |wth| to which the events are actually sent.
      const ui::LocatedEvent* native_event =
          static_cast<const ui::LocatedEvent*>(event->native_event());
      location = native_event->root_location();
      location.Offset(-wth->GetBoundsInPixels().x(),
                      -wth->GetBoundsInPixels().y());
    } else {
      location = event->location();
      // In order to get the correct point in screen coordinates
      // during passive grab, we first need to find on which host window
      // the mouse is on, and find out the screen coordinates on that
      // host window, then convert it back to this host window's coordinate.
      screen_position_client->ConvertHostPointToScreen(root_window, &location);
      screen_position_client->ConvertPointFromScreen(root_window, &location);
      wth->ConvertDIPToPixels(&location);
    }

    event->set_location(location);
    event->set_root_location(location);
  }
}

// static
std::unique_ptr<AshWindowTreeHost> AshWindowTreeHost::Create(
    const AshWindowTreeHostInitParams& init_params) {
  std::unique_ptr<AshWindowTreeHost> ash_window_tree_host =
      ShellPort::Get()->CreateAshWindowTreeHost(init_params);
  if (ash_window_tree_host)
    return ash_window_tree_host;

  if (init_params.offscreen) {
    return std::make_unique<AshWindowTreeHostUnified>(
        init_params.initial_bounds);
  }
  return std::make_unique<AshWindowTreeHostPlatform>(
      init_params.initial_bounds);
}

}  // namespace ash
