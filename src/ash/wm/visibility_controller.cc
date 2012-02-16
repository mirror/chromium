// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/visibility_controller.h"

#include "ash/wm/window_animations.h"
#include "ui/aura/window.h"
#include "ui/aura/window_property.h"

DECLARE_WINDOW_PROPERTY_TYPE(bool)

namespace ash {
namespace internal {
namespace {

// Property set on all windows whose child windows' visibility changes are
// animated.
const aura::WindowProperty<bool>
    kChildWindowVisibilityChangesAnimatedProp = {false};
const aura::WindowProperty<bool>* const
    kChildWindowVisibilityChangesAnimatedKey =
        &kChildWindowVisibilityChangesAnimatedProp;

bool GetChildWindowVisibilityChangesAnimated(aura::Window* window) {
  if (!window)
    return false;
  return window->GetProperty(kChildWindowVisibilityChangesAnimatedKey);
}

}  // namespace

VisibilityController::VisibilityController() {
}

VisibilityController::~VisibilityController() {
}

void VisibilityController::UpdateLayerVisibility(aura::Window* window,
                                                 bool visible) {
  bool animated = GetChildWindowVisibilityChangesAnimated(window->parent()) &&
                  window->type() != aura::client::WINDOW_TYPE_CONTROL &&
                  window->type() != aura::client::WINDOW_TYPE_UNKNOWN;
  animated = animated && AnimateOnChildWindowVisibilityChanged(window, visible);

  if (!visible) {
    // For window hiding animation, we want to check if the window is already
    // animating, and not do SetVisible(false) if it is.
    // TODO(vollick): remove this.
    animated = animated || (window->layer()->GetAnimator()->
        IsAnimatingProperty(ui::LayerAnimationElement::OPACITY) &&
        window->layer()->GetTargetOpacity() == 0.0f);
  }

  // When a window is made visible, we always make its layer visible
  // immediately. When a window is hidden, the layer must be left visible and
  // only made not visible once the animation is complete.
  if (!animated || visible)
    window->layer()->SetVisible(visible);
}

}  // namespace internal

void SetChildWindowVisibilityChangesAnimated(aura::Window* window) {
  window->SetProperty(internal::kChildWindowVisibilityChangesAnimatedKey, true);
}

}  // namespace ash
