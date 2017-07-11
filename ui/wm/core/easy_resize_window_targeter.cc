// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/easy_resize_window_targeter.h"

#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets_f.h"
#include "ui/gfx/geometry/rect.h"

namespace wm {

EasyResizeWindowTargeter::EasyResizeWindowTargeter(
    aura::Window* container,
    const gfx::Insets& mouse_extend,
    const gfx::Insets& touch_extend)
    : container_(container) {
  SetInsets(mouse_extend, touch_extend);
}

EasyResizeWindowTargeter::~EasyResizeWindowTargeter() {}

void EasyResizeWindowTargeter::SetInsets(const gfx::Insets& mouse_inset,
                                         const gfx::Insets& touch_inset) {
  if (mouse_inset == mouse_extend() && touch_inset == touch_extend())
    return;
  WindowTargeter::SetInsets(mouse_inset, touch_inset);
  if (aura::Env::GetInstance()->mode() != aura::Env::Mode::MUS)
    return;

  aura::WindowPortMus::Get(container_)
      ->SetExtendedHitRegionForChildren(mouse_extend(), touch_extend());
}

bool EasyResizeWindowTargeter::EventLocationInsideBounds(
    aura::Window* target,
    const ui::LocatedEvent& event) const {
  return WindowTargeter::EventLocationInsideBounds(target, event);
}

bool EasyResizeWindowTargeter::ShouldUseExtendedBounds(
    const aura::Window* window) const {
  // Use the extended bounds only for immediate child windows of |container_|.
  // Use the default targeter otherwise.
  if (window->parent() != container_)
    return false;

  aura::client::TransientWindowClient* transient_window_client =
      aura::client::GetTransientWindowClient();
  return !transient_window_client ||
      !transient_window_client->GetTransientParent(window) ||
      transient_window_client->GetTransientParent(window) == container_;
}

}  // namespace wm
