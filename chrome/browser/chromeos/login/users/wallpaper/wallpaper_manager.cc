// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/users/wallpaper/wallpaper_manager.h"

#include <utility>

#include "ash/ash_constants.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/shell.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/customization/customization_wallpaper_util.h"
#include "chrome/browser/chromeos/extensions/wallpaper_manager_util.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/users/avatar/user_image_loader.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/image_decoder.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/account_id/account_id.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_image/user_image.h"
#include "components/user_manager/user_names.h"
#include "components/user_manager/user_type.h"
#include "components/wallpaper/wallpaper_files_id.h"
#include "components/wallpaper/wallpaper_resizer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "crypto/sha2.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skia_util.h"
#include "url/gurl.h"

using content::BrowserThread;
using wallpaper::WallpaperInfo;

namespace chromeos {

namespace {

WallpaperManager* wallpaper_manager = nullptr;

// The Wallpaper App that the user is using right now on Chrome OS. It's the
// app that is used when the user right clicks on desktop and selects "Set
// wallpaper" or when the user selects "Set wallpaper" from chrome://settings
// page. This is recorded at user login. It's used to back an UMA histogram so
// it should be treated as append-only.
enum WallpaperAppsType {
  WALLPAPERS_PICKER_APP_CHROMEOS = 0,
  WALLPAPERS_APP_ANDROID = 1,
  WALLPAPERS_APPS_NUM = 2,
};

// Returns index of the first public session user found in |users|
// or -1 otherwise.
int FindPublicSession(const user_manager::UserList& users) {
  int index = -1;
  int i = 0;
  for (user_manager::UserList::const_iterator it = users.begin();
       it != users.end(); ++it, ++i) {
    if ((*it)->GetType() == user_manager::USER_TYPE_PUBLIC_ACCOUNT) {
      index = i;
      break;
    }
  }

  return index;
}

// Returns true if |users| contains users other than device local account users.
bool HasNonDeviceLocalAccounts(const user_manager::UserList& users) {
  for (const auto* user : users) {
    if (!policy::IsDeviceLocalAccountUser(user->GetAccountId().GetUserEmail(),
                                          nullptr))
      return true;
  }
  return false;
}

// A helper to set the wallpaper image for Classic Ash and Mash.
void SetWallpaper(const gfx::ImageSkia& image, wallpaper::WallpaperInfo info) {
  if (ash_util::IsRunningInMash()) {
    // In mash, connect to the WallpaperController interface via mojo.
    service_manager::Connector* connector =
        content::ServiceManagerConnection::GetForProcess()->GetConnector();
    if (!connector)
      return;

    ash::mojom::WallpaperControllerPtr wallpaper_controller;
    connector->BindInterface(ash::mojom::kServiceName, &wallpaper_controller);
    // TODO(crbug.com/655875): Optimize ash wallpaper transport; avoid sending
    // large bitmaps over Mojo; use shared memory like BitmapUploader, etc.
    wallpaper_controller->SetWallpaper(*image.bitmap(), info);
  } else if (ash::Shell::HasInstance()) {
    // Note: Wallpaper setting is skipped in unit tests without shell instances.
    // In classic ash, interact with the WallpaperController class directly.
    ash::Shell::Get()->wallpaper_controller()->SetWallpaperImage(image, info);
  }
}

// A helper function to check the existing/downloaded device wallpaper file's
// hash value matches with the hash value provided in the policy settings.
bool CheckDeviceWallpaperMatchHash(const base::FilePath& device_wallpaper_file,
                                   const std::string& hash) {
  std::string image_data;
  if (base::ReadFileToString(device_wallpaper_file, &image_data)) {
    std::string sha_hash = crypto::SHA256HashString(image_data);
    if (base::ToLowerASCII(base::HexEncode(
            sha_hash.c_str(), sha_hash.size())) == base::ToLowerASCII(hash)) {
      return true;
    }
  }
  return false;
}

}  // namespace

void AssertCalledOnWallpaperSequence(base::SequencedTaskRunner* task_runner) {
#if DCHECK_IS_ON()
  DCHECK(task_runner->RunsTasksInCurrentSequence());
#endif
}

const char kWallpaperSequenceTokenName[] = "wallpaper-sequence";

// WallpaperManager, public: ---------------------------------------------------

WallpaperManager::~WallpaperManager() {
  show_user_name_on_signin_subscription_.reset();
  device_wallpaper_image_subscription_.reset();
  weak_factory_.InvalidateWeakPtrs();
}

// static
void WallpaperManager::Initialize() {
  CHECK(!wallpaper_manager);
  wallpaper_manager = new WallpaperManager();
}

// static
WallpaperManager* WallpaperManager::Get() {
  DCHECK(wallpaper_manager);
  return wallpaper_manager;
}

// static
void WallpaperManager::Shutdown() {
  CHECK(wallpaper_manager);
  delete wallpaper_manager;
  wallpaper_manager = nullptr;
}

void WallpaperManager::ShowUserWallpaper(const AccountId& account_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Some unit tests come here without a UserManager or without a pref system.
  if (!user_manager::UserManager::IsInitialized() ||
      !g_browser_process->local_state()) {
    return;
  }

  const user_manager::User* user =
      user_manager::UserManager::Get()->FindUser(account_id);

  // User is unknown or there is no visible wallpaper in kiosk mode.
  if (!user || user->GetType() == user_manager::USER_TYPE_KIOSK_APP ||
      user->GetType() == user_manager::USER_TYPE_ARC_KIOSK_APP) {
    return;
  }

  // For a enterprise managed user, set the device wallpaper if we're at the
  // login screen.
  if (!user_manager::UserManager::Get()->IsUserLoggedIn() &&
      SetDeviceWallpaperIfApplicable(account_id))
    return;

  // TODO(crbug.com/776464): Move the above to
  // |WallpaperController::ShowUserWallpaper| after
  // |SetDeviceWallpaperIfApplicable| is migrated.
  WallpaperControllerClient::Get()->ShowUserWallpaper(account_id);
}

void WallpaperManager::ShowSigninWallpaper() {
  if (SetDeviceWallpaperIfApplicable(user_manager::SignInAccountId()))
    return;
  WallpaperControllerClient::Get()->ShowSigninWallpaper();
}

void WallpaperManager::InitializeWallpaper() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Apply device customization.
  if (customization_wallpaper_util::ShouldUseCustomizedDefaultWallpaper()) {
    SetCustomizedDefaultWallpaperPaths(
        customization_wallpaper_util::GetCustomizedDefaultWallpaperPath(
            ash::WallpaperController::kSmallWallpaperSuffix),
        customization_wallpaper_util::GetCustomizedDefaultWallpaperPath(
            ash::WallpaperController::kLargeWallpaperSuffix));
  }

  base::CommandLine* command_line = GetCommandLine();
  if (command_line->HasSwitch(chromeos::switches::kGuestSession)) {
    // Guest wallpaper should be initialized when guest login.
    // Note: This maybe called before login. So IsLoggedInAsGuest can not be
    // used here to determine if current user is guest.
    return;
  }

  if (command_line->HasSwitch(::switches::kTestType))
    WizardController::SetZeroDelays();

  // Zero delays is also set in autotests.
  if (WizardController::IsZeroDelayEnabled()) {
    // Ensure tests have some sort of wallpaper.
    ash::Shell::Get()->wallpaper_controller()->CreateEmptyWallpaper();
    return;
  }

  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  if (!user_manager->IsUserLoggedIn()) {
    if (!StartupUtils::IsDeviceRegistered())
      ShowSigninWallpaper();
    else
      InitializeRegisteredDeviceWallpaper();
    return;
  }
  ShowUserWallpaper(user_manager->GetActiveUser()->GetAccountId());
}

bool WallpaperManager::GetLoggedInUserWallpaperInfo(WallpaperInfo* info) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (user_manager::UserManager::Get()->IsLoggedInAsStub()) {
    info->location = "";
    info->layout = wallpaper::WALLPAPER_LAYOUT_CENTER_CROPPED;
    info->type = wallpaper::DEFAULT;
    info->date = base::Time::Now().LocalMidnight();
    *GetCachedWallpaperInfo() = *info;
    return true;
  }

  return GetUserWallpaperInfo(
      user_manager::UserManager::Get()->GetActiveUser()->GetAccountId(), info);
}

void WallpaperManager::SetCustomizedDefaultWallpaperPaths(
    const base::FilePath& default_small_wallpaper_file,
    const base::FilePath& default_large_wallpaper_file) {
  if (!ash::Shell::HasInstance() || ash_util::IsRunningInMash())
    return;
  ash::Shell::Get()->wallpaper_controller()->SetCustomizedDefaultWallpaperPaths(
      default_small_wallpaper_file, default_large_wallpaper_file);
}

void WallpaperManager::AddObservers() {
  show_user_name_on_signin_subscription_ =
      CrosSettings::Get()->AddSettingsObserver(
          kAccountsPrefShowUserNamesOnSignIn,
          base::Bind(&WallpaperManager::InitializeRegisteredDeviceWallpaper,
                     weak_factory_.GetWeakPtr()));
  device_wallpaper_image_subscription_ =
      CrosSettings::Get()->AddSettingsObserver(
          kDeviceWallpaperImage,
          base::Bind(&WallpaperManager::OnDeviceWallpaperPolicyChanged,
                     weak_factory_.GetWeakPtr()));
}

void WallpaperManager::EnsureLoggedInUserWallpaperLoaded() {
  WallpaperInfo info;
  if (GetLoggedInUserWallpaperInfo(&info)) {
    UMA_HISTOGRAM_ENUMERATION("Ash.Wallpaper.Type", info.type,
                              wallpaper::WALLPAPER_TYPE_COUNT);
    RecordWallpaperAppType();
    if (info == *GetCachedWallpaperInfo())
      return;
  }
  ShowUserWallpaper(
      user_manager::UserManager::Get()->GetActiveUser()->GetAccountId());
}

bool WallpaperManager::IsPolicyControlled(const AccountId& account_id) const {
  if (!ash::Shell::HasInstance() || ash_util::IsRunningInMash()) {
    // Some unit tests come here without a Shell instance.
    // TODO(crbug.com/776464): This is intended not to work under mash. Make it
    // work again after WallpaperManager is removed.
    return false;
  }
  bool is_persistent =
      !user_manager::UserManager::Get()->IsUserNonCryptohomeDataEphemeral(
          account_id);
  return ash::Shell::Get()->wallpaper_controller()->IsPolicyControlled(
      account_id, is_persistent);
}

// WallpaperManager, private: --------------------------------------------------

WallpaperManager::WallpaperManager() : weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});
}

void WallpaperManager::SetUserWallpaperInfo(const AccountId& account_id,
                                            const WallpaperInfo& info,
                                            bool is_persistent) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!ash::Shell::HasInstance() || ash_util::IsRunningInMash()) {
    // Some unit tests come here without a Shell instance.
    // TODO(crbug.com/776464): This is intended not to work under mash. Make it
    // work again after WallpaperManager is removed.
    return;
  }
  ash::Shell::Get()->wallpaper_controller()->SetUserWallpaperInfo(
      account_id, info, is_persistent);
}

bool WallpaperManager::SetDeviceWallpaperIfApplicable(
    const AccountId& account_id) {
  std::string url;
  std::string hash;
  if (ShouldSetDeviceWallpaper(account_id, &url, &hash)) {
    // Check if the device wallpaper exists and matches the hash. If so, use it
    // directly. Otherwise download it first.
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::Bind(&base::PathExists,
                   ash::WallpaperController::GetDeviceWallpaperFilePath()),
        base::Bind(&WallpaperManager::OnDeviceWallpaperExists,
                   weak_factory_.GetWeakPtr(), account_id, url, hash));
    return true;
  }
  return false;
}

base::CommandLine* WallpaperManager::GetCommandLine() {
  base::CommandLine* command_line =
      command_line_for_testing_ ? command_line_for_testing_
                                : base::CommandLine::ForCurrentProcess();
  return command_line;
}

void WallpaperManager::InitializeRegisteredDeviceWallpaper() {
  if (user_manager::UserManager::Get()->IsUserLoggedIn())
    return;

  bool disable_boot_animation =
      GetCommandLine()->HasSwitch(switches::kDisableBootAnimation);
  bool show_users = true;
  bool result = CrosSettings::Get()->GetBoolean(
      kAccountsPrefShowUserNamesOnSignIn, &show_users);
  DCHECK(result) << "Unable to fetch setting "
                 << kAccountsPrefShowUserNamesOnSignIn;
  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetUsers();
  int public_session_user_index = FindPublicSession(users);
  if ((!show_users && public_session_user_index == -1) ||
      !HasNonDeviceLocalAccounts(users)) {
    // Boot into the sign in screen.
    ShowSigninWallpaper();
    return;
  }

  if (!disable_boot_animation) {
    int index = public_session_user_index != -1 ? public_session_user_index : 0;
    // Normal boot, load user wallpaper.
    // If normal boot animation is disabled wallpaper would be set
    // asynchronously once user pods are loaded.
    ShowUserWallpaper(users[index]->GetAccountId());
  }
}

bool WallpaperManager::GetUserWallpaperInfo(const AccountId& account_id,
                                            WallpaperInfo* info) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!ash::Shell::HasInstance() || ash_util::IsRunningInMash()) {
    // Some unit tests come here without a Shell instance.
    // TODO(crbug.com/776464): This is intended not to work under mash. Make it
    // work again after WallpaperManager is removed.
    return false;
  }
  bool is_persistent =
      !user_manager::UserManager::Get()->IsUserNonCryptohomeDataEphemeral(
          account_id);
  return ash::Shell::Get()->wallpaper_controller()->GetUserWallpaperInfo(
      account_id, info, is_persistent);
}

bool WallpaperManager::ShouldSetDeviceWallpaper(const AccountId& account_id,
                                                std::string* url,
                                                std::string* hash) {
  // Only allow the device wallpaper for enterprise managed devices.
  if (!g_browser_process->platform_part()
           ->browser_policy_connector_chromeos()
           ->IsEnterpriseManaged()) {
    return false;
  }

  const base::DictionaryValue* dict = nullptr;
  if (!CrosSettings::Get()->GetDictionary(kDeviceWallpaperImage, &dict) ||
      !dict->GetStringWithoutPathExpansion("url", url) ||
      !dict->GetStringWithoutPathExpansion("hash", hash)) {
    return false;
  }

  // Only set the device wallpaper if we're at the login screen.
  if (user_manager::UserManager::Get()->IsUserLoggedIn())
    return false;

  return true;
}

void WallpaperManager::SetDefaultWallpaperImpl(const AccountId& account_id,
                                               bool show_wallpaper) {
  if (!ash::Shell::HasInstance() || ash_util::IsRunningInMash()) {
    // Some unit tests come here without a Shell instance.
    // TODO(crbug.com/776464): This is intended not to work under mash. Make it
    // work again after WallpaperManager is removed.
    WallpaperInfo info("", wallpaper::WALLPAPER_LAYOUT_STRETCH,
                       wallpaper::DEFAULT, base::Time::Now().LocalMidnight());
    SetWallpaper(ash::WallpaperController::CreateSolidColorWallpaper(), info);
    return;
  }
  const user_manager::User* user =
      user_manager::UserManager::Get()->FindUser(account_id);
  if (!user) {
    LOG(ERROR) << "User of " << account_id
               << " cannot be found. This should never happen"
               << " within WallpaperManager::SetDefaultWallpaperImpl.";
    return;
  }
  ash::Shell::Get()->wallpaper_controller()->SetDefaultWallpaperImpl(
      account_id, user->GetType(), show_wallpaper);
}

void WallpaperManager::RecordWallpaperAppType() {
  user_manager::User* user = user_manager::UserManager::Get()->GetActiveUser();
  Profile* profile = ProfileHelper::Get()->GetProfileByUser(user);

  UMA_HISTOGRAM_ENUMERATION(
      "Ash.Wallpaper.Apps",
      wallpaper_manager_util::ShouldUseAndroidWallpapersApp(profile)
          ? WALLPAPERS_APP_ANDROID
          : WALLPAPERS_PICKER_APP_CHROMEOS,
      WALLPAPERS_APPS_NUM);
}

const char* WallpaperManager::GetCustomWallpaperSubdirForCurrentResolution() {
  ash::WallpaperController::WallpaperResolution resolution =
      ash::WallpaperController::GetAppropriateResolution();
  return resolution == ash::WallpaperController::WALLPAPER_RESOLUTION_SMALL
             ? ash::WallpaperController::kSmallWallpaperSubDir
             : ash::WallpaperController::kLargeWallpaperSubDir;
}

void WallpaperManager::OnDeviceWallpaperPolicyChanged() {
  SetDeviceWallpaperIfApplicable(
      user_manager::UserManager::Get()->IsUserLoggedIn()
          ? user_manager::UserManager::Get()->GetActiveUser()->GetAccountId()
          : user_manager::SignInAccountId());
}

void WallpaperManager::OnDeviceWallpaperExists(const AccountId& account_id,
                                               const std::string& url,
                                               const std::string& hash,
                                               bool exist) {
  if (exist) {
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::Bind(&CheckDeviceWallpaperMatchHash,
                   ash::WallpaperController::GetDeviceWallpaperFilePath(),
                   hash),
        base::Bind(&WallpaperManager::OnCheckDeviceWallpaperMatchHash,
                   weak_factory_.GetWeakPtr(), account_id, url, hash));
  } else {
    GURL device_wallpaper_url(url);
    device_wallpaper_downloader_.reset(new CustomizationWallpaperDownloader(
        g_browser_process->system_request_context(), device_wallpaper_url,
        ash::WallpaperController::GetDeviceWallpaperDir(),
        ash::WallpaperController::GetDeviceWallpaperFilePath(),
        base::Bind(&WallpaperManager::OnDeviceWallpaperDownloaded,
                   weak_factory_.GetWeakPtr(), account_id, hash)));
    device_wallpaper_downloader_->Start();
  }
}

void WallpaperManager::OnDeviceWallpaperDownloaded(const AccountId& account_id,
                                                   const std::string& hash,
                                                   bool success,
                                                   const GURL& url) {
  if (!success) {
    LOG(ERROR) << "Failed to download the device wallpaper. Fallback to "
                  "default wallpaper.";
    SetDefaultWallpaperImpl(account_id, true /*show_wallpaper=*/);
    return;
  }

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::Bind(&CheckDeviceWallpaperMatchHash,
                 ash::WallpaperController::GetDeviceWallpaperFilePath(), hash),
      base::Bind(&WallpaperManager::OnCheckDeviceWallpaperMatchHash,
                 weak_factory_.GetWeakPtr(), account_id, url.spec(), hash));
}

void WallpaperManager::OnCheckDeviceWallpaperMatchHash(
    const AccountId& account_id,
    const std::string& url,
    const std::string& hash,
    bool match) {
  if (!match) {
    if (retry_download_if_failed_) {
      // We only retry to download the device wallpaper one more time if the
      // hash doesn't match.
      retry_download_if_failed_ = false;
      GURL device_wallpaper_url(url);
      device_wallpaper_downloader_.reset(new CustomizationWallpaperDownloader(
          g_browser_process->system_request_context(), device_wallpaper_url,
          ash::WallpaperController::GetDeviceWallpaperDir(),
          ash::WallpaperController::GetDeviceWallpaperFilePath(),
          base::Bind(&WallpaperManager::OnDeviceWallpaperDownloaded,
                     weak_factory_.GetWeakPtr(), account_id, hash)));
      device_wallpaper_downloader_->Start();
    } else {
      LOG(ERROR) << "The device wallpaper hash doesn't match with provided "
                    "hash value. Fallback to default wallpaper! ";
      SetDefaultWallpaperImpl(account_id, true /*show_wallpaper=*/);

      // Reset the boolean variable so that it can retry to download when the
      // next device wallpaper request comes in.
      retry_download_if_failed_ = true;
    }
    return;
  }

  const base::FilePath file_path =
      ash::WallpaperController::GetDeviceWallpaperFilePath();
    user_image_loader::StartWithFilePath(
        task_runner_, ash::WallpaperController::GetDeviceWallpaperFilePath(),
        ImageDecoder::ROBUST_JPEG_CODEC,
        0,  // Do not crop.
        base::Bind(&WallpaperManager::OnDeviceWallpaperDecoded,
                   weak_factory_.GetWeakPtr(), account_id));
}

void WallpaperManager::OnDeviceWallpaperDecoded(
    const AccountId& account_id,
    std::unique_ptr<user_manager::UserImage> user_image) {
  // It might be possible that the device policy controlled wallpaper finishes
  // decoding after the user logs in. In this case do nothing.
  if (!user_manager::UserManager::Get()->IsUserLoggedIn()) {
    WallpaperInfo wallpaper_info = {
        ash::WallpaperController::GetDeviceWallpaperFilePath().value(),
        wallpaper::WALLPAPER_LAYOUT_CENTER_CROPPED, wallpaper::DEVICE,
        base::Time::Now().LocalMidnight()};
    // TODO(crbug.com/776464): This should go through PendingWallpaper after
    // moving to WallpaperController.
    SetWallpaper(user_image->image(), wallpaper_info);
  }
}

wallpaper::WallpaperInfo* WallpaperManager::GetCachedWallpaperInfo() {
  if (!ash::Shell::HasInstance() || ash_util::IsRunningInMash())
    return &dummy_current_user_wallpaper_info_;
  return ash::Shell::Get()
      ->wallpaper_controller()
      ->GetCurrentUserWallpaperInfo();
}

}  // namespace chromeos
