// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host_common.h"

#include "ash/shell.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/app_launch_controller.h"
#include "chrome/browser/chromeos/login/arc_kiosk_controller.h"
#include "chrome/browser/chromeos/login/demo_mode/demo_app_launcher.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/mobile_config.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/system/device_disabling_manager.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/browser/ui/webui/chromeos/internet_detail_dialog.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "ui/wm/public/scoped_drag_drop_disabler.h"

namespace chromeos {
namespace {

// The delay of triggering initialization of the device policy subsystem
// after the login screen is initialized. This makes sure that device policy
// network requests are made while the system is idle waiting for user input.
constexpr int64_t kPolicyServiceInitializationDelayMilliseconds = 100;

}  // namespace

LoginDisplayHostCommon::LoginDisplayHostCommon() : weak_factory_(this) {
  keep_alive_.reset(
      new ScopedKeepAlive(KeepAliveOrigin::LOGIN_DISPLAY_HOST_WEBUI,
                          KeepAliveRestartOption::DISABLED));

  // Disable Drag'n'Drop for the login session.
  // ash::Shell may be null in tests.
  if (ash::Shell::HasInstance() && !ash_util::IsRunningInMash()) {
    scoped_drag_drop_disabler_.reset(
        new wm::ScopedDragDropDisabler(ash::Shell::GetPrimaryRootWindow()));
  } else {
    NOTIMPLEMENTED();
  }

  // Close the login screen on NOTIFICATION_APP_TERMINATING.
  registrar_.Add(this, chrome::NOTIFICATION_APP_TERMINATING,
                 content::NotificationService::AllSources());
  // NOTIFICATION_BROWSER_OPENED is issued after browser is created, but
  // not shown yet. Lock window has to be closed at this point so that
  // a browser window exists and the window can acquire input focus.
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_OPENED,
                 content::NotificationService::AllSources());
}

LoginDisplayHostCommon::~LoginDisplayHostCommon() {}

void LoginDisplayHostCommon::BeforeSessionStart() {
  session_starting_ = true;
}

AppLaunchController* LoginDisplayHostCommon::GetAppLaunchController() {
  return app_launch_controller_.get();
}

void LoginDisplayHostCommon::StartSignInScreen(
    const LoginScreenContext& context) {
  PrewarmAuthentication();

  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetUsers();

  // Fix for users who updated device and thus never passed register screen.
  // If we already have users, we assume that it is not a second part of
  // OOBE. See http://crosbug.com/6289
  if (!StartupUtils::IsDeviceRegistered() && !users.empty()) {
    VLOG(1) << "Mark device registered because there are remembered users: "
            << users.size();
    StartupUtils::MarkDeviceRegistered(base::OnceClosure());
  }

  // Initiate mobile config load.
  MobileConfig::GetInstance();

  // Initiate device policy fetching.
  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  connector->ScheduleServiceInitialization(
      kPolicyServiceInitializationDelayMilliseconds);

  // Run UI-specific logic.
  OnStartSignInScreen(context);

  // Enable status area after starting sign-in screen, as it may depend on the
  // UI being visible.
  SetStatusAreaVisible(true);
}

void LoginDisplayHostCommon::PrewarmAuthentication() {
  auth_prewarmer_ = std::make_unique<AuthPrewarmer>();
  auth_prewarmer_->PrewarmAuthentication(base::BindOnce(
      &LoginDisplayHostCommon::OnAuthPrewarmDone, weak_factory_.GetWeakPtr()));
}

void LoginDisplayHostCommon::StartAppLaunch(const std::string& app_id,
                                            bool diagnostic_mode,
                                            bool is_auto_launch) {
  VLOG(1) << "Login >> start app launch.";
  SetStatusAreaVisible(false);

  // Wait for the |CrosSettings| to become either trusted or permanently
  // untrusted.
  const CrosSettingsProvider::TrustedStatus status =
      CrosSettings::Get()->PrepareTrustedValues(base::Bind(
          &LoginDisplayHostCommon::StartAppLaunch, weak_factory_.GetWeakPtr(),
          app_id, diagnostic_mode, is_auto_launch));
  if (status == CrosSettingsProvider::TEMPORARILY_UNTRUSTED)
    return;

  if (status == CrosSettingsProvider::PERMANENTLY_UNTRUSTED) {
    // If the |CrosSettings| are permanently untrusted, refuse to launch a
    // single-app kiosk mode session.
    LOG(ERROR) << "Login >> Refusing to launch single-app kiosk mode.";
    SetStatusAreaVisible(true);
    return;
  }

  if (system::DeviceDisablingManager::IsDeviceDisabledDuringNormalOperation()) {
    // If the device is disabled, bail out. A device disabled screen will be
    // shown by the DeviceDisablingManager.
    return;
  }

  OnStartAppLaunch();

  app_launch_controller_ = std::make_unique<AppLaunchController>(
      app_id, diagnostic_mode, this, GetOobeUI());

  app_launch_controller_->StartAppLaunch(is_auto_launch);
}

void LoginDisplayHostCommon::StartDemoAppLaunch() {
  VLOG(1) << "Login >> starting demo app.";
  SetStatusAreaVisible(false);

  demo_app_launcher_ = std::make_unique<DemoAppLauncher>();
  demo_app_launcher_->StartDemoAppLaunch();
}

void LoginDisplayHostCommon::StartArcKiosk(const AccountId& account_id) {
  VLOG(1) << "Login >> start ARC kiosk.";
  SetStatusAreaVisible(false);
  arc_kiosk_controller_ =
      std::make_unique<ArcKioskController>(this, GetOobeUI());
  arc_kiosk_controller_->StartArcKiosk(account_id);

  OnStartArcKiosk();
}

void LoginDisplayHostCommon::CompleteLogin(const UserContext& user_context) {
  ExistingUserController* controller =
      ExistingUserController::current_controller();
  if (controller)
    controller->CompleteLogin(user_context);
}

void LoginDisplayHostCommon::OnGaiaScreenReady() {
  ExistingUserController* controller =
      ExistingUserController::current_controller();
  if (controller)
    controller->OnGaiaScreenReady();
}

void LoginDisplayHostCommon::SetDisplayEmail(const std::string& email) {
  ExistingUserController* controller =
      ExistingUserController::current_controller();
  if (controller)
    controller->SetDisplayEmail(email);
}

void LoginDisplayHostCommon::SetDisplayAndGivenName(
    const std::string& display_name,
    const std::string& given_name) {
  ExistingUserController* controller =
      ExistingUserController::current_controller();
  if (controller)
    controller->SetDisplayAndGivenName(display_name, given_name);
}

void LoginDisplayHostCommon::LoadWallpaper(const AccountId& account_id) {
  WallpaperControllerClient::Get()->ShowUserWallpaper(account_id);
}

void LoginDisplayHostCommon::LoadSigninWallpaper() {
  WallpaperControllerClient::Get()->ShowSigninWallpaper();
}

bool LoginDisplayHostCommon::IsUserWhitelisted(const AccountId& account_id) {
  ExistingUserController* controller =
      ExistingUserController::current_controller();
  if (!controller)
    return true;
  return controller->IsUserWhitelisted(account_id);
}

void LoginDisplayHostCommon::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_APP_TERMINATING) {
    ShutdownDisplayHost();
  } else if (type == chrome::NOTIFICATION_BROWSER_OPENED && session_starting_) {
    // Browsers created before session start (windows opened by extensions, for
    // example) are ignored.
    OnBrowserCreated();
    registrar_.Remove(this, chrome::NOTIFICATION_APP_TERMINATING,
                      content::NotificationService::AllSources());
    registrar_.Remove(this, chrome::NOTIFICATION_BROWSER_OPENED,
                      content::NotificationService::AllSources());
  }
}

void LoginDisplayHostCommon::OnAuthPrewarmDone() {
  auth_prewarmer_.reset();
}

void LoginDisplayHostCommon::ShutdownDisplayHost() {
  if (shutting_down_)
    return;

  shutting_down_ = true;
  registrar_.RemoveAll();
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

}  // namespace chromeos
