// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/wallpaper_controller_client.h"

#include "ash/public/interfaces/wallpaper.mojom.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {

// Simulates WallpaperController in ash.
// TODO(crbug.com/776464): This class will most likely be extracted to a
// separate file, and the functions will increment a counter.
class TestWallpaperController : ash::mojom::WallpaperController {
 public:
  TestWallpaperController() : binding_(this) {}

  ~TestWallpaperController() override = default;

  bool was_client_set() const { return was_client_set_; }

  // Returns a mojo interface pointer bound to this object.
  ash::mojom::WallpaperControllerPtr CreateInterfacePtr() {
    ash::mojom::WallpaperControllerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // ash::mojom::WallpaperController:
  void SetClient(ash::mojom::WallpaperControllerClientPtr client) override {
    was_client_set_ = true;
  }

  void SetCustomWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                          const std::string& wallpaper_files_id,
                          const std::string& file,
                          wallpaper::WallpaperLayout layout,
                          wallpaper::WallpaperType type,
                          const SkBitmap& image,
                          bool show_wallpaper) override {
    NOTIMPLEMENTED();
  }

  void SetOnlineWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                          const SkBitmap& image,
                          const std::string& url,
                          wallpaper::WallpaperLayout layout,
                          bool show_wallpaper) override {
    NOTIMPLEMENTED();
  }

  void SetDefaultWallpaper(ash::mojom::WallpaperUserInfoPtr user_info,
                           bool show_wallpaper) override {
    NOTIMPLEMENTED();
  }

  void SetCustomizedDefaultWallpaper(
      const GURL& wallpaper_url,
      const base::FilePath& downloaded_file,
      const base::FilePath& resized_directory) override {
    NOTIMPLEMENTED();
  }

  void ShowUserWallpaper(ash::mojom::WallpaperUserInfoPtr user_info) override {
    NOTIMPLEMENTED();
  }

  void ShowSigninWallpaper() override { NOTIMPLEMENTED(); }

  void RemoveUserWallpaper(
      ash::mojom::WallpaperUserInfoPtr user_info) override {
    NOTIMPLEMENTED();
  }

  void SetWallpaper(const SkBitmap& wallpaper,
                    const wallpaper::WallpaperInfo& wallpaper_info) override {
    NOTIMPLEMENTED();
  }

  void AddObserver(
      ash::mojom::WallpaperObserverAssociatedPtrInfo observer) override {
    NOTIMPLEMENTED();
  }

  void GetWallpaperColors(GetWallpaperColorsCallback callback) override {
    NOTIMPLEMENTED();
  }

 private:
  mojo::Binding<ash::mojom::WallpaperController> binding_;

  bool was_client_set_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestWallpaperController);
};

class WallpaperControllerClientTest : public testing::Test {
 public:
  WallpaperControllerClientTest() = default;
  ~WallpaperControllerClientTest() override = default;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(WallpaperControllerClientTest);
};

TEST_F(WallpaperControllerClientTest, Construction) {
  WallpaperControllerClient client;
  TestWallpaperController controller;
  client.InitForTesting(controller.CreateInterfacePtr());
  client.FlushForTesting();

  // Singleton was initialized.
  EXPECT_EQ(&client, WallpaperControllerClient::Get());

  // Object was set as client.
  EXPECT_TRUE(controller.was_client_set());
}

}  // namespace