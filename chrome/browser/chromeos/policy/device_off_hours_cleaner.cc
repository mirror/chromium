// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"

namespace policy {

namespace em = enterprise_management;

em::ChromeDeviceSettingsProto DeviceOffHoursController::clear_policies(
    em::ChromeDeviceSettingsProto* input_policies,
    std::set<std::string> off_policies) {
  em::ChromeDeviceSettingsProto policies;
  policies.CopyFrom(*input_policies);
  if (off_policies.find("device_policy_refresh_rate") != off_policies.end()) {
    policies.clear_device_policy_refresh_rate();
  }
  if (off_policies.find("user_whitelist") != off_policies.end()) {
    policies.clear_user_whitelist();
  }
  if (off_policies.find("guest_mode_enabled") != off_policies.end()) {
    policies.clear_guest_mode_enabled();
  }
  if (off_policies.find("device_proxy_settings") != off_policies.end()) {
    policies.clear_device_proxy_settings();
  }
  if (off_policies.find("camera_enabled") != off_policies.end()) {
    policies.clear_camera_enabled();
  }
  if (off_policies.find("show_user_names") != off_policies.end()) {
    policies.clear_show_user_names();
  }
  if (off_policies.find("data_roaming_enabled") != off_policies.end()) {
    policies.clear_data_roaming_enabled();
  }
  if (off_policies.find("allow_new_users") != off_policies.end()) {
    policies.clear_allow_new_users();
  }
  if (off_policies.find("metrics_enabled") != off_policies.end()) {
    policies.clear_metrics_enabled();
  }
  if (off_policies.find("release_channel") != off_policies.end()) {
    policies.clear_release_channel();
  }
  if (off_policies.find("open_network_configuration") != off_policies.end()) {
    policies.clear_open_network_configuration();
  }
  if (off_policies.find("device_reporting") != off_policies.end()) {
    policies.clear_device_reporting();
  }
  if (off_policies.find("ephemeral_users_enabled") != off_policies.end()) {
    policies.clear_ephemeral_users_enabled();
  }
  if (off_policies.find("app_pack") != off_policies.end()) {
    policies.clear_app_pack();
  }
  if (off_policies.find("forced_logout_timeouts") != off_policies.end()) {
    policies.clear_forced_logout_timeouts();
  }
  if (off_policies.find("login_screen_saver") != off_policies.end()) {
    policies.clear_login_screen_saver();
  }
  if (off_policies.find("auto_update_settings") != off_policies.end()) {
    policies.clear_auto_update_settings();
  }
  if (off_policies.find("start_up_urls") != off_policies.end()) {
    policies.clear_start_up_urls();
  }
  if (off_policies.find("pinned_apps") != off_policies.end()) {
    policies.clear_pinned_apps();
  }
  if (off_policies.find("system_timezone") != off_policies.end()) {
    policies.clear_system_timezone();
  }
  if (off_policies.find("device_local_accounts") != off_policies.end()) {
    policies.clear_device_local_accounts();
  }
  if (off_policies.find("allow_redeem_offers") != off_policies.end()) {
    policies.clear_allow_redeem_offers();
  }
  if (off_policies.find("start_up_flags") != off_policies.end()) {
    policies.clear_start_up_flags();
  }
  if (off_policies.find("uptime_limit") != off_policies.end()) {
    policies.clear_uptime_limit();
  }
  if (off_policies.find("variations_parameter") != off_policies.end()) {
    policies.clear_variations_parameter();
  }
  if (off_policies.find("attestation_settings") != off_policies.end()) {
    policies.clear_attestation_settings();
  }
  if (off_policies.find("accessibility_settings") != off_policies.end()) {
    policies.clear_accessibility_settings();
  }
  if (off_policies.find("supervised_users_settings") != off_policies.end()) {
    policies.clear_supervised_users_settings();
  }
  if (off_policies.find("login_screen_power_management") !=
      off_policies.end()) {
    policies.clear_login_screen_power_management();
  }
  if (off_policies.find("use_24hour_clock") != off_policies.end()) {
    policies.clear_use_24hour_clock();
  }
  if (off_policies.find("auto_clean_up_settings") != off_policies.end()) {
    policies.clear_auto_clean_up_settings();
  }
  if (off_policies.find("system_settings") != off_policies.end()) {
    policies.clear_system_settings();
  }
  if (off_policies.find("saml_settings") != off_policies.end()) {
    policies.clear_saml_settings();
  }
  if (off_policies.find("reboot_on_shutdown") != off_policies.end()) {
    policies.clear_reboot_on_shutdown();
  }
  if (off_policies.find("device_heartbeat_settings") != off_policies.end()) {
    policies.clear_device_heartbeat_settings();
  }
  if (off_policies.find("extension_cache_size") != off_policies.end()) {
    policies.clear_extension_cache_size();
  }
  if (off_policies.find("login_screen_domain_auto_complete") !=
      off_policies.end()) {
    policies.clear_login_screen_domain_auto_complete();
  }
  if (off_policies.find("device_log_upload_settings") != off_policies.end()) {
    policies.clear_device_log_upload_settings();
  }
  if (off_policies.find("display_rotation_default") != off_policies.end()) {
    policies.clear_display_rotation_default();
  }
  if (off_policies.find("allow_kiosk_app_control_chrome_version") !=
      off_policies.end()) {
    policies.clear_allow_kiosk_app_control_chrome_version();
  }
  if (off_policies.find("login_authentication_behavior") !=
      off_policies.end()) {
    policies.clear_login_authentication_behavior();
  }
  if (off_policies.find("usb_detachable_whitelist") != off_policies.end()) {
    policies.clear_usb_detachable_whitelist();
  }
  if (off_policies.find("allow_bluetooth") != off_policies.end()) {
    policies.clear_allow_bluetooth();
  }
  if (off_policies.find("quirks_download_enabled") != off_policies.end()) {
    policies.clear_quirks_download_enabled();
  }
  if (off_policies.find("login_video_capture_allowed_urls") !=
      off_policies.end()) {
    policies.clear_login_video_capture_allowed_urls();
  }
  if (off_policies.find("device_login_screen_app_install_list") !=
      off_policies.end()) {
    policies.clear_device_login_screen_app_install_list();
  }
  if (off_policies.find("network_throttling") != off_policies.end()) {
    policies.clear_network_throttling();
  }
  if (off_policies.find("device_wallpaper_image") != off_policies.end()) {
    policies.clear_device_wallpaper_image();
  }
  if (off_policies.find("login_screen_locales") != off_policies.end()) {
    policies.clear_login_screen_locales();
  }
  if (off_policies.find("login_screen_input_methods") != off_policies.end()) {
    policies.clear_login_screen_input_methods();
  }
  if (off_policies.find("device_ecryptfs_migration_strategy") !=
      off_policies.end()) {
    policies.clear_device_ecryptfs_migration_strategy();
  }
  if (off_policies.find("device_second_factor_authentication") !=
      off_policies.end()) {
    policies.clear_device_second_factor_authentication();
  }
  if (off_policies.find("cast_receiver_name") != off_policies.end()) {
    policies.clear_cast_receiver_name();
  }
  if (off_policies.find("device_off_hours") != off_policies.end()) {
    policies.clear_device_off_hours();
  }
  return policies;
}
}  // namespace policy
