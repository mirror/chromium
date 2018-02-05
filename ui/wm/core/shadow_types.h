// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_SHADOW_TYPES_H_
#define UI_WM_CORE_SHADOW_TYPES_H_

#include "ui/aura/window.h"
#include "ui/wm/core/wm_core_export.h"

namespace wm {

enum StandardShadowElevation {
  // default.
  DEFAULT = -1,
  NONE = 0,
  TINY = 2,
  SMALL = 6,
  MEDIUM = 8,
  LARGE = 24,
};

struct ShadowElevation {
  explicit constexpr ShadowElevation(int elevation) : value(elevation) {}
  constexpr operator int() const { return value; }

  int value;
};

// Indicates an elevation should be chosen based on the window.
constexpr ShadowElevation kShadowElevationDefault = ShadowElevation(-1);

// Different types of drop shadows that can be drawn under a window by the
// shell. Used as a value for the kShadowTypeKey property.
constexpr ShadowElevation kInfoBubbleShadowElevation = ShadowElevation(2);
constexpr ShadowElevation kMenuOrTooltipShadowElevation = ShadowElevation(6);
constexpr ShadowElevation kInactiveNormalShadowElevation = ShadowElevation(8);
constexpr ShadowElevation kOverlayWindowShadowElevation = ShadowElevation(8);
constexpr ShadowElevation kActiveNormalShadowElevation = ShadowElevation(24);

WM_CORE_EXPORT void SetShadowElevation(aura::Window* window,
                                       ShadowElevation elevation);

// A property key describing the drop shadow that should be displayed under the
// window. A null value is interpreted as using the default.
WM_CORE_EXPORT extern const aura::WindowProperty<ShadowElevation>* const
    kShadowElevationKey;

}  // namespace wm

// Declaring the template specialization here to make sure that the
// compiler in all builds, including jumbo builds, always knows about
// the specialization before the first template instance use (for
// instance in shadw_controller.cc). Using a template instance before
// its specialization is declared in a translation unit is a C++
// error.
DECLARE_EXPORTED_UI_CLASS_PROPERTY_TYPE(WM_CORE_EXPORT, ::wm::ShadowElevation);

#endif  // UI_WM_CORE_SHADOW_TYPES_H_
