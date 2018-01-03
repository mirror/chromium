// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_USERS_WALLPAPER_WALLPAPER_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_USERS_WALLPAPER_WALLPAPER_MANAGER_H_

#include <stddef.h>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "ash/wallpaper/wallpaper_controller.h"
#include "base/containers/circular_deque.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/customization/customization_wallpaper_downloader.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "components/signin/core/account_id/account_id.h"
#include "components/user_manager/user_image/user_image.h"
#include "components/wallpaper/wallpaper_export.h"
#include "components/wallpaper/wallpaper_info.h"
#include "ui/gfx/image/image_skia.h"

namespace base {
class CommandLine;
class SequencedTaskRunner;
}  // namespace base

namespace user_manager {
class UserImage;
}  // namespace user_manager

namespace chromeos {

// Asserts that the current task is sequenced with any other task that calls
// this.
void AssertCalledOnWallpaperSequence(base::SequencedTaskRunner* task_runner);

// Name of wallpaper sequence token.
extern const char kWallpaperSequenceTokenName[];

class WallpaperManager {
 public:
  ~WallpaperManager();

  // Expects there is no instance of WallpaperManager and create one.
  static void Initialize();

  // Gets pointer to singleton WallpaperManager instance.
  static WallpaperManager* Get();

  // Deletes the existing instance of WallpaperManager. Allows the
  // WallpaperManager to remove any observers it has registered.
  static void Shutdown();

  // Initializes wallpaper. If logged in, loads user's wallpaper. If not logged
  // in, uses a solid color wallpaper. If logged in as a stub user, uses an
  // empty wallpaper.
  void InitializeWallpaper();

  // Gets wallpaper information of logged in user.
  bool GetLoggedInUserWallpaperInfo(wallpaper::WallpaperInfo* info);

  // TODO(crbug.com/776464): Convert it to mojo call.
  // A wrapper of |WallpaperController::SetCustomizedDefaultWallpaperPaths|.
  void SetCustomizedDefaultWallpaperPaths(
      const base::FilePath& default_small_wallpaper_file,
      const base::FilePath& default_large_wallpaper_file);

  // Adds |this| as an observer to various settings.
  void AddObservers();

  // Loads wallpaper asynchronously if the current wallpaper is not the
  // wallpaper of logged in user.
  void EnsureLoggedInUserWallpaperLoaded();

  // A wrapper of |WallpaperController::IsPolicyControlled|.
  bool IsPolicyControlled(const AccountId& account_id) const;

 private:
  friend class WallpaperManagerPolicyTest;

  WallpaperManager();

  // A wrapper of |WallpaperController::SetUserWallpaperInfo|.
  void SetUserWallpaperInfo(const AccountId& account_id,
                            const wallpaper::WallpaperInfo& info,
                            bool is_persistent);

  // If the device is enterprise managed and the device wallpaper policy exists,
  // set the device wallpaper as the login screen wallpaper.
  bool SetDeviceWallpaperIfApplicable(const AccountId& account_id);

  // A wrapper of |WallpaperController::GetPathFromCache|.
  bool GetPathFromCache(const AccountId& account_id, base::FilePath* path);

  // The number of wallpapers have loaded. For test only.
  int loaded_wallpapers_for_test() const;

  // Gets the CommandLine representing the current process's command line.
  base::CommandLine* GetCommandLine();

  // Initialize wallpaper of registered device after device policy is trusted.
  // Note that before device is enrolled, it proceeds with untrusted setting.
  void InitializeRegisteredDeviceWallpaper();

  // A wrapper of |WallpaperController::GetUserWallpaperInfo|.
  bool GetUserWallpaperInfo(const AccountId& account_id,
                            wallpaper::WallpaperInfo* info) const;

  // Returns true if the device wallpaper should be set for the account.
  bool ShouldSetDeviceWallpaper(const AccountId& account_id,
                                std::string* url,
                                std::string* hash);

  // A wrapper of |WallpaperController::SetDefaultWallpaperImpl|.
  void SetDefaultWallpaperImpl(const AccountId& account_id,
                               bool show_wallpaper);

  // Record the Wallpaper App that the user is using right now on Chrome OS.
  void RecordWallpaperAppType();

  // Returns wallpaper subdirectory name for current resolution.
  const char* GetCustomWallpaperSubdirForCurrentResolution();

  // This is called when the device wallpaper policy changes.
  void OnDeviceWallpaperPolicyChanged();
  // This is call after checking if the device wallpaper exists.
  void OnDeviceWallpaperExists(const AccountId& account_id,
                               const std::string& url,
                               const std::string& hash,
                               bool exist);
  // This is called after the device wallpaper is download (either successful or
  // failed).
  void OnDeviceWallpaperDownloaded(const AccountId& account_id,
                                   const std::string& hash,
                                   bool success,
                                   const GURL& url);
  // Check if the device wallpaper matches the hash that's provided in the
  // device wallpaper policy setting.
  void OnCheckDeviceWallpaperMatchHash(const AccountId& account_id,
                                       const std::string& url,
                                       const std::string& hash,
                                       bool match);
  // This is called when the device wallpaper is decoded successfully.
  void OnDeviceWallpaperDecoded(
      const AccountId& account_id,
      std::unique_ptr<user_manager::UserImage> user_image);

  // Returns the cached logged-in user wallpaper info, or a dummy value under
  // mash.
  wallpaper::WallpaperInfo* GetCachedWallpaperInfo();

  std::unique_ptr<CrosSettings::ObserverSubscription>
      show_user_name_on_signin_subscription_;

  std::unique_ptr<CrosSettings::ObserverSubscription>
      device_wallpaper_image_subscription_;
  std::unique_ptr<CustomizationWallpaperDownloader>
      device_wallpaper_downloader_;

  // The number of loaded wallpapers.
  int loaded_wallpapers_for_test_ = 0;

  base::ThreadChecker thread_checker_;

  // Wallpaper sequenced task runner.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // If non-NULL, used in place of the real command line.
  base::CommandLine* command_line_for_testing_ = nullptr;

  // A placeholder for |current_user_wallpaper_info_| under mash.
  wallpaper::WallpaperInfo dummy_current_user_wallpaper_info_;

  bool should_cache_wallpaper_ = false;

  bool retry_download_if_failed_ = true;

  base::WeakPtrFactory<WallpaperManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WallpaperManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_USERS_WALLPAPER_WALLPAPER_MANAGER_H_
