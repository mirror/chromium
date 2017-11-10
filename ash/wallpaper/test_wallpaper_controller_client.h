// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WALLPAPER_TEST_WALLPAPER_CONTROLLER_CLIENT_H_
#define ASH_WALLPAPER_TEST_WALLPAPER_CONTROLLER_CLIENT_H_

#include "ash/public/interfaces/wallpaper.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ash {

class TestWallpaperControllerClient : public mojom::WallpaperControllerClient {
 public:
  TestWallpaperControllerClient();

  ~TestWallpaperControllerClient() override;

  mojom::WallpaperControllerClientPtr CreateInterfacePtrAndBind();

  // mojom::WallpaperControllerClient:
  MOCK_METHOD0(OpenWallpaperPicker, void());

 private:
  mojo::Binding<mojom::WallpaperControllerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestWallpaperControllerClient);
};

}  // namespace ash

#endif  // ASH_WALLPAPER_TEST_WALLPAPER_CONTROLLER_CLIENT_H_