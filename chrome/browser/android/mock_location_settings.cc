// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/mock_location_settings.h"

bool MockLocationSettings::has_android_location_permission_ = false;
bool MockLocationSettings::is_system_location_setting_enabled_ = false;
bool MockLocationSettings::can_prompt_for_android_location_permission_ = false;
bool MockLocationSettings::location_settings_dialog_enabled_ = false;
LocationSettingsDialogOutcome
    MockLocationSettings::location_settings_dialog_outcome_ = NO_PROMPT;
bool MockLocationSettings::has_shown_location_settings_dialog_ = false;
bool MockLocationSettings::resolve_location_settings_dialog_async_ = false;
LocationSettings::LocationSettingsDialogOutcomeCallback
    MockLocationSettings::location_settings_dialog_callback_ =
        LocationSettingsDialogOutcomeCallback();

MockLocationSettings::MockLocationSettings() : LocationSettings() {
}

MockLocationSettings::~MockLocationSettings() {
}

void MockLocationSettings::SetLocationStatus(
    bool has_android_location_permission,
    bool is_system_location_setting_enabled) {
  has_android_location_permission_ = has_android_location_permission;
  is_system_location_setting_enabled_ = is_system_location_setting_enabled;
}

void MockLocationSettings::SetCanPromptForAndroidPermission(bool can_prompt) {
  can_prompt_for_android_location_permission_ = can_prompt;
}

void MockLocationSettings::SetLocationSettingsDialogStatus(
    bool enabled,
    LocationSettingsDialogOutcome outcome) {
  location_settings_dialog_enabled_ = enabled;
  location_settings_dialog_outcome_ = outcome;
}

bool MockLocationSettings::HasShownLocationSettingsDialog() {
  return has_shown_location_settings_dialog_;
}

void MockLocationSettings::ClearHasShownLocationSettingsDialog() {
  has_shown_location_settings_dialog_ = false;
}

void MockLocationSettings::SetAsyncLocationSettingsDialog() {
  resolve_location_settings_dialog_async_ = true;
}

void MockLocationSettings::ResolveAsyncLocationSettingsDialog() {
  std::move(location_settings_dialog_callback_)
      .Run(location_settings_dialog_outcome_);
}

bool MockLocationSettings::HasAndroidLocationPermission() {
  return has_android_location_permission_;
}

bool MockLocationSettings::CanPromptForAndroidLocationPermission(
    content::WebContents* web_contents) {
  return can_prompt_for_android_location_permission_;
}

bool MockLocationSettings::IsSystemLocationSettingEnabled() {
  return is_system_location_setting_enabled_;
}

bool MockLocationSettings::CanPromptToEnableSystemLocationSetting() {
  return location_settings_dialog_enabled_;
}

void MockLocationSettings::PromptToEnableSystemLocationSetting(
    const LocationSettingsDialogContext prompt_context,
    content::WebContents* web_contents,
    LocationSettingsDialogOutcomeCallback callback) {
  has_shown_location_settings_dialog_ = true;

  if (resolve_location_settings_dialog_async_) {
    location_settings_dialog_callback_ = std::move(callback);
  } else {
    std::move(callback).Run(location_settings_dialog_outcome_);
  }
}
