// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_off_hours_controller.h"

namespace chromeos {

namespace em = enterprise_management;

em::ChromeDeviceSettingsProto DeviceOffHoursController::clear_policies(
    em::ChromeDeviceSettingsProto* input_policies,
    std::set<std::string> off_policies) {
  em::ChromeDeviceSettingsProto policies;
  policies.CopyFrom(*input_policies);
  if (off_policies.find("DevicePolicyRefreshRateProto") != off_policies.end()) {
    policies.clear_device_policy_refresh_rate();
  }
  if (off_policies.find("UserWhitelistProto") != off_policies.end()) {
    policies.clear_user_whitelist();
  }
  if (off_policies.find("GuestModeEnabledProto") != off_policies.end()) {
    policies.clear_guest_mode_enabled();
  }
  if (off_policies.find("OBSOLETE_DeviceProxySettingsProto") !=
      off_policies.end()) {
    policies.clear_device_proxy_settings();
  }
  if (off_policies.find("CameraEnabledProto") != off_policies.end()) {
    policies.clear_camera_enabled();
  }
  if (off_policies.find("ShowUserNamesOnSigninProto") != off_policies.end()) {
    policies.clear_show_user_names();
  }
  if (off_policies.find("DataRoamingEnabledProto") != off_policies.end()) {
    policies.clear_data_roaming_enabled();
  }
  if (off_policies.find("AllowNewUsersProto") != off_policies.end()) {
    policies.clear_allow_new_users();
  }
  if (off_policies.find("MetricsEnabledProto") != off_policies.end()) {
    policies.clear_metrics_enabled();
  }
  if (off_policies.find("ReleaseChannelProto") != off_policies.end()) {
    policies.clear_release_channel();
  }
  if (off_policies.find("DeviceOpenNetworkConfigurationProto") !=
      off_policies.end()) {
    policies.clear_open_network_configuration();
  }
  if (off_policies.find("DeviceReportingProto") != off_policies.end()) {
    policies.clear_device_reporting();
  }
  if (off_policies.find("EphemeralUsersEnabledProto") != off_policies.end()) {
    policies.clear_ephemeral_users_enabled();
  }
  if (off_policies.find("OBSOLETE_AppPackProto") != off_policies.end()) {
    policies.clear_app_pack();
  }
  if (off_policies.find("OBSOLETE_ForcedLogoutTimeoutsProto") !=
      off_policies.end()) {
    policies.clear_forced_logout_timeouts();
  }
  if (off_policies.find("OBSOLETE_ScreenSaverProto") != off_policies.end()) {
    policies.clear_login_screen_saver();
  }
  if (off_policies.find("AutoUpdateSettingsProto") != off_policies.end()) {
    policies.clear_auto_update_settings();
  }
  if (off_policies.find("OBSOLETE_StartUpUrlsProto") != off_policies.end()) {
    policies.clear_start_up_urls();
  }
  if (off_policies.find("OBSOLETE_PinnedAppsProto") != off_policies.end()) {
    policies.clear_pinned_apps();
  }
  if (off_policies.find("SystemTimezoneProto") != off_policies.end()) {
    policies.clear_system_timezone();
  }
  if (off_policies.find("DeviceLocalAccountsProto") != off_policies.end()) {
    policies.clear_device_local_accounts();
  }
  if (off_policies.find("AllowRedeemChromeOsRegistrationOffersProto") !=
      off_policies.end()) {
    policies.clear_allow_redeem_offers();
  }
  if (off_policies.find("StartUpFlagsProto") != off_policies.end()) {
    policies.clear_start_up_flags();
  }
  if (off_policies.find("UptimeLimitProto") != off_policies.end()) {
    policies.clear_uptime_limit();
  }
  if (off_policies.find("VariationsParameterProto") != off_policies.end()) {
    policies.clear_variations_parameter();
  }
  if (off_policies.find("AttestationSettingsProto") != off_policies.end()) {
    policies.clear_attestation_settings();
  }
  if (off_policies.find("AccessibilitySettingsProto") != off_policies.end()) {
    policies.clear_accessibility_settings();
  }
  if (off_policies.find("SupervisedUsersSettingsProto") != off_policies.end()) {
    policies.clear_supervised_users_settings();
  }
  if (off_policies.find("LoginScreenPowerManagementProto") !=
      off_policies.end()) {
    policies.clear_login_screen_power_management();
  }
  if (off_policies.find("SystemUse24HourClockProto") != off_policies.end()) {
    policies.clear_use_24hour_clock();
  }
  if (off_policies.find("AutoCleanupSettigsProto") != off_policies.end()) {
    policies.clear_auto_clean_up_settings();
  }
  if (off_policies.find("SystemSettingsProto") != off_policies.end()) {
    policies.clear_system_settings();
  }
  if (off_policies.find("SAMLSettingsProto") != off_policies.end()) {
    policies.clear_saml_settings();
  }
  if (off_policies.find("RebootOnShutdownProto") != off_policies.end()) {
    policies.clear_reboot_on_shutdown();
  }
  if (off_policies.find("DeviceHeartbeatSettingsProto") != off_policies.end()) {
    policies.clear_device_heartbeat_settings();
  }
  if (off_policies.find("ExtensionCacheSizeProto") != off_policies.end()) {
    policies.clear_extension_cache_size();
  }
  if (off_policies.find("LoginScreenDomainAutoCompleteProto") !=
      off_policies.end()) {
    policies.clear_login_screen_domain_auto_complete();
  }
  if (off_policies.find("DeviceLogUploadSettingsProto") != off_policies.end()) {
    policies.clear_device_log_upload_settings();
  }
  if (off_policies.find("DisplayRotationDefaultProto") != off_policies.end()) {
    policies.clear_display_rotation_default();
  }
  if (off_policies.find("AllowKioskAppControlChromeVersionProto") !=
      off_policies.end()) {
    policies.clear_allow_kiosk_app_control_chrome_version();
  }
  if (off_policies.find("LoginAuthenticationBehaviorProto") !=
      off_policies.end()) {
    policies.clear_login_authentication_behavior();
  }
  if (off_policies.find("UsbDetachableWhitelistProto") != off_policies.end()) {
    policies.clear_usb_detachable_whitelist();
  }
  if (off_policies.find("AllowBluetoothProto") != off_policies.end()) {
    policies.clear_allow_bluetooth();
  }
  if (off_policies.find("DeviceQuirksDownloadEnabledProto") !=
      off_policies.end()) {
    policies.clear_quirks_download_enabled();
  }
  if (off_policies.find("LoginVideoCaptureAllowedUrlsProto") !=
      off_policies.end()) {
    policies.clear_login_video_capture_allowed_urls();
  }
  if (off_policies.find("DeviceLoginScreenAppInstallListProto") !=
      off_policies.end()) {
    policies.clear_device_login_screen_app_install_list();
  }
  if (off_policies.find("NetworkThrottlingEnabledProto") !=
      off_policies.end()) {
    policies.clear_network_throttling();
  }
  if (off_policies.find("DeviceWallpaperImageProto") != off_policies.end()) {
    policies.clear_device_wallpaper_image();
  }
  if (off_policies.find("LoginScreenLocalesProto") != off_policies.end()) {
    policies.clear_login_screen_locales();
  }
  if (off_policies.find("LoginScreenInputMethodsProto") != off_policies.end()) {
    policies.clear_login_screen_input_methods();
  }
  if (off_policies.find("DeviceEcryptfsMigrationStrategyProto") !=
      off_policies.end()) {
    policies.clear_device_ecryptfs_migration_strategy();
  }
  if (off_policies.find("DeviceSecondFactorAuthenticationProto") !=
      off_policies.end()) {
    policies.clear_device_second_factor_authentication();
  }
  if (off_policies.find("DeviceOffHoursProto") != off_policies.end()) {
    policies.clear_device_off_hours();
  }
  return policies;
}
}  // namespace chromeos
