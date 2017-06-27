// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WALLPAPER_WALLPAPER_COLOR_PROFILE_H_
#define COMPONENTS_WALLPAPER_WALLPAPER_COLOR_PROFILE_H_

namespace wallpaper {

// The color profile index, ordered as the color profiles applied in
// ash::WallpaperController. This enum is used to get the corresponding
// prominent color from calculation results.
enum ColorProfileIndex {
  COLOR_PROFILE_INDEX_DARK_VIBRANT = 0,
  COLOR_PROFILE_INDEX_NORMAL_VIBRANT,
  COLOR_PROFILE_INDEX_LIGHT_VIBRANT,
  COLOR_PROFILE_INDEX_DARK_MUTED,
  COLOR_PROFILE_INDEX_NORMAL_MUTED,
  COLOR_PROFILE_INDEX_LIGHT_MUTED,

  NUM_OF_COLOR_PROFILES,
};

}  // namespace wallpaper

#endif  // COMPONENTS_WALLPAPER_WALLPAPER_COLOR_PROFILE_H_
