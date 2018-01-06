// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/wallpaper_controller_client.h"

#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/shell.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ash/wallpaper/wallpaper_window_state_manager.h"
#include "base/path_service.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/chromeos/customization/customization_wallpaper_util.h"
#include "chrome/browser/chromeos/extensions/wallpaper_manager_util.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/common/chrome_paths.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "components/user_manager/known_user.h"
#include "components/user_manager/user_manager.h"
#include "components/wallpaper/wallpaper_files_id.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Known user keys.
const char kWallpaperFilesId[] = "wallpaper-files-id";

WallpaperControllerClient* g_instance = nullptr;

// Creates a mojom::WallpaperUserInfo for the account id. Returns nullptr if
// user manager cannot find the user.
ash::mojom::WallpaperUserInfoPtr AccountIdToWallpaperUserInfo(
    const AccountId& account_id) {
  if (!account_id.is_valid()) {
    // |account_id| may be invalid in tests.
    return nullptr;
  }
  const user_manager::User* user =
      user_manager::UserManager::Get()->FindUser(account_id);
  if (!user)
    return nullptr;

  ash::mojom::WallpaperUserInfoPtr wallpaper_user_info =
      ash::mojom::WallpaperUserInfo::New();
  wallpaper_user_info->account_id = account_id;
  wallpaper_user_info->type = user->GetType();
  wallpaper_user_info->is_ephemeral =
      user_manager::UserManager::Get()->IsUserNonCryptohomeDataEphemeral(
          account_id);
  wallpaper_user_info->has_gaia_account = user->HasGaiaAccount();

  return wallpaper_user_info;
}

// This has once been copied from
// brillo::cryptohome::home::SanitizeUserName(username) to be used for
// wallpaper identification purpose only.
//
// Historic note: We need some way to identify users wallpaper files in
// the device filesystem. Historically User::username_hash() was used for this
// purpose, but it has two caveats:
// 1. username_hash() is defined only after user has logged in.
// 2. If cryptohome identifier changes, username_hash() will also change,
//    and we may lose user => wallpaper files mapping at that point.
// So this function gives WallpaperManager independent hashing method to break
// this dependency.
wallpaper::WallpaperFilesId HashWallpaperFilesIdStr(
    const std::string& files_id_unhashed) {
  chromeos::SystemSaltGetter* salt_getter = chromeos::SystemSaltGetter::Get();
  DCHECK(salt_getter);

  // System salt must be defined at this point.
  const chromeos::SystemSaltGetter::RawSalt* salt = salt_getter->GetRawSalt();
  if (!salt)
    LOG(FATAL) << "WallpaperManager HashWallpaperFilesIdStr(): no salt!";

  unsigned char binmd[base::kSHA1Length];
  std::string lowercase(files_id_unhashed);
  std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                 ::tolower);
  std::vector<uint8_t> data = *salt;
  std::copy(files_id_unhashed.begin(), files_id_unhashed.end(),
            std::back_inserter(data));
  base::SHA1HashBytes(data.data(), data.size(), binmd);
  std::string result = base::HexEncode(binmd, sizeof(binmd));
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return wallpaper::WallpaperFilesId::FromString(result);
}

// Returns true if wallpaper files id can be returned successfully.
bool CanGetFilesId() {
  return chromeos::SystemSaltGetter::IsInitialized() &&
         chromeos::SystemSaltGetter::Get()->GetRawSalt();
}

// Calls |callback| when system salt is ready. (|CanGetFilesId| returns true.)
void AddCanGetFilesIdCallback(const base::Closure& callback) {
  // System salt may not be initialized in tests.
  if (chromeos::SystemSaltGetter::IsInitialized())
    chromeos::SystemSaltGetter::Get()->AddOnSystemSaltReady(callback);
}

// Returns true if |users| contains users other than device local accounts.
bool HasNonDeviceLocalAccounts(const user_manager::UserList& users) {
  for (const auto* user : users) {
    if (!policy::IsDeviceLocalAccountUser(user->GetAccountId().GetUserEmail(),
                                          nullptr))
      return true;
  }
  return false;
}

// Returns index of the first public session user found in |users| or -1 if
// there's none.
int FindPublicSession(const user_manager::UserList& users) {
  for (size_t i = 0; i < users.size(); ++i) {
    if (users[i]->GetType() == user_manager::USER_TYPE_PUBLIC_ACCOUNT)
      return i;
  }
  return -1;
}

}  // namespace

WallpaperControllerClient::WallpaperControllerClient()
    : policy_handler_(this),
      binding_(this),
      activation_client_observer_(this),
      window_observer_(this),
      weak_factory_(this) {
  DCHECK(!g_instance);
  g_instance = this;
}

WallpaperControllerClient::~WallpaperControllerClient() {
  show_user_name_on_signin_subscription_.reset();
  weak_factory_.InvalidateWeakPtrs();
  DCHECK_EQ(this, g_instance);
  g_instance = nullptr;
}

void WallpaperControllerClient::Init() {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &wallpaper_controller_);
  BindAndSetClient();

  show_user_name_on_signin_subscription_ =
      chromeos::CrosSettings::Get()->AddSettingsObserver(
          chromeos::kAccountsPrefShowUserNamesOnSignIn,
          base::Bind(&WallpaperControllerClient::ShowRegisteredDeviceWallpaper,
                     weak_factory_.GetWeakPtr()));
}

void WallpaperControllerClient::InitForTesting(
    ash::mojom::WallpaperControllerPtr controller) {
  wallpaper_controller_ = std::move(controller);
  BindAndSetClient();
}

// static
WallpaperControllerClient* WallpaperControllerClient::Get() {
  return g_instance;
}

wallpaper::WallpaperFilesId WallpaperControllerClient::GetFilesId(
    const AccountId& account_id) const {
  DCHECK(CanGetFilesId());
  std::string stored_value;
  if (user_manager::known_user::GetStringPref(account_id, kWallpaperFilesId,
                                              &stored_value)) {
    return wallpaper::WallpaperFilesId::FromString(stored_value);
  }

  const wallpaper::WallpaperFilesId wallpaper_files_id =
      HashWallpaperFilesIdStr(account_id.GetUserEmail());
  user_manager::known_user::SetStringPref(account_id, kWallpaperFilesId,
                                          wallpaper_files_id.id());
  return wallpaper_files_id;
}

void WallpaperControllerClient::SetCustomWallpaper(
    const AccountId& account_id,
    const wallpaper::WallpaperFilesId& wallpaper_files_id,
    const std::string& file_name,
    wallpaper::WallpaperLayout layout,
    const gfx::ImageSkia& image,
    bool show_wallpaper) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->SetCustomWallpaper(
      std::move(user_info), wallpaper_files_id.id(), file_name, layout,
      *image.bitmap(), show_wallpaper);
}

void WallpaperControllerClient::SetOnlineWallpaper(
    const AccountId& account_id,
    const gfx::ImageSkia& image,
    const std::string& url,
    wallpaper::WallpaperLayout layout,
    bool show_wallpaper) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->SetOnlineWallpaper(
      std::move(user_info), *image.bitmap(), url, layout, show_wallpaper);
}

void WallpaperControllerClient::SetDefaultWallpaper(const AccountId& account_id,
                                                    bool show_wallpaper) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone setting the wallpaper until we can get files id.
  if (!CanGetFilesId()) {
    LOG(WARNING)
        << "Cannot get wallpaper files id in SetDefaultWallpaper. This "
           "should never happen under normal circumstances.";
    AddCanGetFilesIdCallback(
        base::Bind(&WallpaperControllerClient::SetDefaultWallpaper,
                   weak_factory_.GetWeakPtr(), account_id, show_wallpaper));
    return;
  }

  wallpaper_controller_->SetDefaultWallpaper(
      std::move(user_info), GetFilesId(account_id).id(), show_wallpaper);
}

void WallpaperControllerClient::SetCustomizedDefaultWallpaperPaths(
    const base::FilePath& customized_default_small_path,
    const base::FilePath& customized_default_large_path) {
  wallpaper_controller_->SetCustomizedDefaultWallpaperPaths(
      customized_default_small_path, customized_default_large_path);
}

void WallpaperControllerClient::SetPolicyWallpaper(
    const AccountId& account_id,
    std::unique_ptr<std::string> data) {
  if (!data)
    return;

  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone setting the wallpaper until we can get files id. See
  // https://crbug.com/615239.
  if (!CanGetFilesId()) {
    AddCanGetFilesIdCallback(base::Bind(
        &WallpaperControllerClient::SetPolicyWallpaper,
        weak_factory_.GetWeakPtr(), account_id, base::Passed(std::move(data))));
    return;
  }

  wallpaper_controller_->SetPolicyWallpaper(std::move(user_info),
                                            GetFilesId(account_id).id(), *data);
}

void WallpaperControllerClient::UpdateCustomWallpaperLayout(
    const AccountId& account_id,
    wallpaper::WallpaperLayout layout) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->UpdateCustomWallpaperLayout(std::move(user_info),
                                                     layout);
}

void WallpaperControllerClient::ShowUserWallpaper(const AccountId& account_id) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;
  wallpaper_controller_->ShowUserWallpaper(std::move(user_info));
}

void WallpaperControllerClient::ShowSigninWallpaper() {
  wallpaper_controller_->ShowSigninWallpaper();
}

void WallpaperControllerClient::RemoveUserWallpaper(
    const AccountId& account_id) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone removing the wallpaper until we can get files id.
  if (!CanGetFilesId()) {
    LOG(WARNING)
        << "Cannot get wallpaper files id in RemoveUserWallpaper. This "
           "should never happen under normal circumstances.";
    AddCanGetFilesIdCallback(
        base::Bind(&WallpaperControllerClient::RemoveUserWallpaper,
                   weak_factory_.GetWeakPtr(), account_id));
    return;
  }

  wallpaper_controller_->RemoveUserWallpaper(std::move(user_info),
                                             GetFilesId(account_id).id());
}

void WallpaperControllerClient::RemovePolicyWallpaper(
    const AccountId& account_id) {
  ash::mojom::WallpaperUserInfoPtr user_info =
      AccountIdToWallpaperUserInfo(account_id);
  if (!user_info)
    return;

  // Postpone removing the wallpaper until we can get files id.
  if (!CanGetFilesId()) {
    LOG(WARNING)
        << "Cannot get wallpaper files id in RemovePolicyWallpaper. This "
           "should never happen under normal circumstances.";
    AddCanGetFilesIdCallback(
        base::Bind(&WallpaperControllerClient::RemovePolicyWallpaper,
                   weak_factory_.GetWeakPtr(), account_id));
    return;
  }

  wallpaper_controller_->RemovePolicyWallpaper(std::move(user_info),
                                               GetFilesId(account_id).id());
}

void WallpaperControllerClient::OpenWallpaperPickerIfAllowed() {
  wallpaper_controller_->OpenWallpaperPickerIfAllowed();
}

void WallpaperControllerClient::IsActiveUserPolicyControlled(
    ash::mojom::WallpaperController::IsActiveUserPolicyControlledCallback
        callback) {
  wallpaper_controller_->IsActiveUserPolicyControlled(std::move(callback));
}

void WallpaperControllerClient::OnDeviceWallpaperChanged() {
  wallpaper_controller_->SetDeviceWallpaperPolicyEnforced(true /*enforced=*/);
}

void WallpaperControllerClient::OnDeviceWallpaperPolicyCleared() {
  wallpaper_controller_->SetDeviceWallpaperPolicyEnforced(false /*enforced=*/);
}

void WallpaperControllerClient::OnWindowActivated(ActivationReason reason,
                                                  aura::Window* gained_active,
                                                  aura::Window* lost_active) {
  if (!gained_active)
    return;

  const std::string arc_wallpapers_app_id = ArcAppListPrefs::GetAppId(
      wallpaper_manager_util::kAndroidWallpapersAppPackage,
      wallpaper_manager_util::kAndroidWallpapersAppActivity);
  ash::ShelfID shelf_id =
      ash::ShelfID::Deserialize(gained_active->GetProperty(ash::kShelfIDKey));
  if (shelf_id.app_id == arc_wallpapers_app_id) {
    ash::WallpaperWindowStateManager::MinimizeInactiveWindows(
        user_manager::UserManager::Get()->GetActiveUser()->username_hash());
    DCHECK(!ash_util::IsRunningInMash() && ash::Shell::Get());
    activation_client_observer_.Remove(ash::Shell::Get()->activation_client());
    window_observer_.Add(gained_active);
  }
}

void WallpaperControllerClient::OnWindowDestroying(aura::Window* window) {
  window_observer_.Remove(window);
  ash::WallpaperWindowStateManager::RestoreWindows(
      user_manager::UserManager::Get()->GetActiveUser()->username_hash());
}

void WallpaperControllerClient::FlushForTesting() {
  wallpaper_controller_.FlushForTesting();
}

void WallpaperControllerClient::BindAndSetClient() {
  ash::mojom::WallpaperControllerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));

  // Get the paths of wallpaper directories.
  base::FilePath user_data_path;
  CHECK(PathService::Get(chrome::DIR_USER_DATA, &user_data_path));
  base::FilePath chromeos_wallpapers_path;
  CHECK(PathService::Get(chrome::DIR_CHROMEOS_WALLPAPERS,
                         &chromeos_wallpapers_path));
  base::FilePath chromeos_custom_wallpapers_path;
  CHECK(PathService::Get(chrome::DIR_CHROMEOS_CUSTOM_WALLPAPERS,
                         &chromeos_custom_wallpapers_path));

  // Set the static variables in WallpaperController to make it work under MASH.
  // The reason we do this is so that the static utility functions in
  // WallpaperController in ash continue to work under MASH.
  // TODO(wzang|xdai): Create a WallpaperPaths class under //ash/public/cpp and
  // move all the unititly functions there. See https://crbug.com/795159.
  ash::WallpaperController::dir_user_data_path_ = user_data_path;
  ash::WallpaperController::dir_chrome_os_wallpapers_path_ =
      chromeos_wallpapers_path;
  ash::WallpaperController::dir_chrome_os_custom_wallpapers_path_ =
      chromeos_custom_wallpapers_path;

  // Apply device customization.
  base::FilePath customized_default_small_path;
  base::FilePath customized_default_large_path;
  if (chromeos::customization_wallpaper_util::
          ShouldUseCustomizedDefaultWallpaper()) {
    customized_default_small_path = chromeos::customization_wallpaper_util::
        GetCustomizedDefaultWallpaperPath(
            ash::WallpaperController::kSmallWallpaperSuffix);
    customized_default_large_path = chromeos::customization_wallpaper_util::
        GetCustomizedDefaultWallpaperPath(
            ash::WallpaperController::kLargeWallpaperSuffix);
  }

  wallpaper_controller_->Init(
      std::move(client), user_data_path, chromeos_wallpapers_path,
      chromeos_custom_wallpapers_path, customized_default_small_path,
      customized_default_large_path,
      policy_handler_.IsDeviceWallpaperPolicyEnforced());
}

void WallpaperControllerClient::ShowRegisteredDeviceWallpaper() {
  if (user_manager::UserManager::Get()->IsUserLoggedIn())
    return;

  bool show_users = true;
  DCHECK(chromeos::CrosSettings::Get()->GetBoolean(
      chromeos::kAccountsPrefShowUserNamesOnSignIn, &show_users))
      << "Unable to fetch setting "
      << chromeos::kAccountsPrefShowUserNamesOnSignIn;

  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetUsers();
  int public_session_user_index = FindPublicSession(users);

  // Show the default signin wallpaper if there's no user to display.
  if ((!show_users && public_session_user_index == -1) ||
      !HasNonDeviceLocalAccounts(users)) {
    ShowSigninWallpaper();
    return;
  }

  // If normal boot animation is disabled, the login screen is responsible to
  // set the wallpaper once user pods are loaded.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kDisableBootAnimation)) {
    int index = public_session_user_index != -1 ? public_session_user_index : 0;
    ShowUserWallpaper(users[index]->GetAccountId());
  }
}

void WallpaperControllerClient::OpenWallpaperPicker() {
  if (wallpaper_manager_util::ShouldUseAndroidWallpapersApp(
          chromeos::ProfileHelper::Get()->GetProfileByUser(
              user_manager::UserManager::Get()->GetActiveUser())) &&
      !ash_util::IsRunningInMash()) {
    // Window activation watch to minimize all inactive windows is only needed
    // by Android Wallpaper app. Legacy Chrome OS Wallpaper Picker app does that
    // via extension API.
    activation_client_observer_.Add(ash::Shell::Get()->activation_client());
  }

  wallpaper_manager_util::OpenWallpaperManager();
}

void WallpaperControllerClient::OnReadyToSetWallpaper() {
  // TODO(crbug.com/776464): Consider deprecating this method after views-based
  // login is enabled. It should be fast enough to request the first wallpaper
  // so that there's no visible delay. In other scenarios such as restart after
  // crash, user manager should request the wallpaper.

  // Guest wallpaper should be initialized when guest logs in.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kGuestSession)) {
    return;
  }

  // Do not set wallpaper in tests.
  if (chromeos::WizardController::IsZeroDelayEnabled())
    return;

  // Show the wallpaper of the active user during an user session.
  if (user_manager::UserManager::Get()->IsUserLoggedIn()) {
    ShowUserWallpaper(
        user_manager::UserManager::Get()->GetActiveUser()->GetAccountId());
    return;
  }

  // If the device is not registered yet (e.g. during OOBE), show the default
  // signin wallpaper.
  if (!chromeos::StartupUtils::IsDeviceRegistered()) {
    ShowSigninWallpaper();
    return;
  }

  ShowRegisteredDeviceWallpaper();
}
