// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wallpaper/test_wallpaper_controller_client.h"

#include "mojo/public/cpp/bindings/interface_request.h"

namespace ash {

TestWallpaperControllerClient::TestWallpaperControllerClient()
    : binding_(this) {}

TestWallpaperControllerClient::~TestWallpaperControllerClient() = default;

mojom::WallpaperControllerClientPtr
TestWallpaperControllerClient::CreateInterfacePtrAndBind() {
  mojom::WallpaperControllerClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

}  // namespace ash