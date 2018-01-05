// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_WALLPAPER_UTILS_H_
#define ASH_PUBLIC_CPP_WALLPAPER_UTILS_H_

#include "ash/public/cpp/ash_public_export.h"

namespace ash {
namespace wallpaper_utils {

// Returns true if the wallpaper setting (used to open the wallpaper picker)
// should be visible.
ASH_PUBLIC_EXPORT bool ShouldShowWallpaperSetting();

}  // namespace wallpaper_utils
}  // namespace ash

#endif  // ASH_PUBLIC_CPP_WALLPAPER_UTILS_H_