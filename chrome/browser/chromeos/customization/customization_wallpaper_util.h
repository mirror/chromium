// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CUSTOMIZATION_CUSTOMIZATION_WALLPAPER_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_CUSTOMIZATION_CUSTOMIZATION_WALLPAPER_UTIL_H_

#include <memory>

class GURL;

namespace base {
class FilePath;
}

namespace gfx {
class ImageSkia;
}

namespace user_manager {
class UserImage;
}

namespace chromeos {
namespace customization_wallpaper_util {

// First checks if the file paths exist for both large and small sizes, then
// calls |SetCustomizedDefaultWallpaperAfterCheck| with |both_sizes_exist|.
void StartSettingCustomizedDefaultWallpaper(
    const GURL& wallpaper_url,
    const base::FilePath& file_path,
    const base::FilePath& resized_directory);

// If |both_sizes_exist| is false or the url doesn't match the current value,
// initiate image decoding, otherwise directly sends the paths.
void SetCustomizedDefaultWallpaperAfterCheck(
    const GURL& wallpaper_url,
    const base::FilePath& file_path,
    const base::FilePath& rescaled_small_path,
    const base::FilePath& rescaled_large_path,
    bool both_sizes_exist);

// Initiates resizing and saving the customized default wallpapers if decoding
// is successful.
void OnCustomizedDefaultWallpaperDecoded(
    const GURL& wallpaper_url,
    const base::FilePath& rescaled_small_path,
    const base::FilePath& rescaled_large_path,
    std::unique_ptr<user_manager::UserImage> wallpaper);

// Resizes and saves the customized default wallpapers.
bool ResizeAndSaveCustomizedDefaultWallpaper(
    std::unique_ptr<gfx::ImageSkia> image,
    const base::FilePath& rescaled_small_path,
    const base::FilePath& rescaled_large_path);

// Check the result of |ResizeAndSaveCustomizedDefaultWallpaper| and sends
// the paths to apply the wallpapers.
void OnCustomizedDefaultWallpaperResizedAndSaved(
    const GURL& wallpaper_url,
    const base::FilePath& rescaled_small_path,
    const base::FilePath& rescaled_large_path,
    bool success);

// Gets the customized default wallpaper file paths for the |suffix|.
base::FilePath GetCustomizedDefaultWallpaperPath(const std::string& suffix);

// Whether customized default wallpaper should be used wherever a default
// wallpaper is needed.
bool ShouldUseCustomizedDefaultWallpaper();

}  // namespace customization_wallpaper_util
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CUSTOMIZATION_CUSTOMIZATION_WALLPAPER_UTIL_H_
