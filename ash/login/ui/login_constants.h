// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_CONSTANTS_H
#define ASH_LOGIN_UI_LOGIN_CONSTANTS_H

#include "ash/ash_export.h"
#include "third_party/skia/include/core/SkColor.h"

namespace ash {

// The default base color of the login/lock screen when the dark muted color
// extracted from wallpaper is invalid.
ASH_EXPORT constexpr SkColor kLoginDefaultBaseColor = SK_ColorBLACK;

// The alpha value for the login/lock screen background.
ASH_EXPORT constexpr int kLoginTranslucentAlpha = 153;

// The alpha value for the scrollable container on the account picker.
ASH_EXPORT constexpr int kLoginScrollTranslucentAlpha = 76;

// The alpha value used to darken the login/lock screen.
ASH_EXPORT constexpr int kLoginTranslucentColorDarkenAlpha = 128;

// The blur sigma for login/lock screen. The blur pixel will be 3 * this value.
ASH_EXPORT constexpr float kLoginBlurSigma = 10.0f;

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_CONSTANTS_H