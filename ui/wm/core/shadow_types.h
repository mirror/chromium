// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_SHADOW_TYPES_H_
#define UI_WM_CORE_SHADOW_TYPES_H_

#include "ui/aura/window.h"
#include "ui/wm/core/wm_core_export.h"

namespace wm {

// Indicates an elevation should be chosen based on the window.
constexpr int kShadowElevationDefault = -1;

// Different types of drop shadows that can be drawn under a window by the
// shell. Used as a value for the kShadowTypeKey property.
constexpr int kShadowElevationNone = 0;
constexpr int kInfoBubbleShadowElevation = 2;
constexpr int kMenuOrTooltipShadowElevation = 6;
constexpr int kInactiveNormalShadowElevation = 8;
constexpr int kOverlayWindowShadowElevation = 8;
constexpr int kActiveNormalShadowElevation = 24;

WM_CORE_EXPORT void SetShadowElevation(aura::Window* window, int elevation);

// A property key describing the drop shadow that should be displayed under the
// window. A null value is interpreted as using the default.
WM_CORE_EXPORT extern const aura::WindowProperty<int>* const
    kShadowElevationKey;

}  // namespace wm

#endif  // UI_WM_CORE_SHADOW_TYPES_H_
