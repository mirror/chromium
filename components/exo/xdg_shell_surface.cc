// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/xdg_shell_surface.h"

#include <algorithm>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "components/exo/surface.h"
#include "components/exo/wm_helper.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/class_property.h"
#include "ui/compositor/compositor.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/path.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/shadow.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_util.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// XdgShellSurface, public:

XdgShellSurface::XdgShellSurface(Surface* surface,
                                 BoundsMode bounds_mode,
                                 const gfx::Point& origin,
                                 bool activatable,
                                 bool can_minimize,
                                 int container)
    : ShellSurface(surface,
                   bounds_mode,
                   origin,
                   activatable,
                   can_minimize,
                   container) {}

XdgShellSurface::~XdgShellSurface() {}

void XdgShellSurface::SetBoundsMode(BoundsMode mode) {
  TRACE_EVENT1("exo", "XdgShellSurface::SetBoundsMode", "mode",
               static_cast<int>(mode));

  bounds_mode_ = mode;
}

////////////////////////////////////////////////////////////////////////////////
// views::WidgetDelegate overrides:

void XdgShellSurface::SaveWindowPlacement(const gfx::Rect& bounds,
                                          ui::WindowShowState show_state) {
  WidgetDelegate::SaveWindowPlacement(bounds, show_state);
}

bool XdgShellSurface::GetSavedWindowPlacement(
    const views::Widget* widget,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return WidgetDelegate::GetSavedWindowPlacement(widget, bounds, show_state);
}

}  // namespace exo
