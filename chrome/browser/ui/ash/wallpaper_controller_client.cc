// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/wallpaper_controller_client.h"

namespace {
WallpaperControllerClient* g_instance = nullptr;
}  // namespace

WallpaperControllerClient::WallpaperControllerClient() : binding_(this) {
  // Bind WallpaperController, set client etc.
}

WallpaperControllerClient::~WallpaperControllerClient() {}

// Wrapper of ash::mojom::WallpaperController interface.
void WallpaperControllerClient::ShowUserWallpaper(const AccountId& account_id) {
  if (/* flag is present */) {
    // Calls to ash directly.
    wallpaper_controller_->ShowUserWallpaper(account_id);
  } else {
    // The current code path.
    WallpaperManager::Get()->ShowUserWallpaper(account_id);
  }
}
