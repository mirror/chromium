// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/test/app/ios_chrome_scoped_testing_block_popups_pref.h"

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#import "ios/chrome/test/app/chrome_test_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSChromeScopedTestingBlockPopupsPref::IOSChromeScopedTestingBlockPopupsPref(
    ContentSetting setting) {
  original_setting_ = GetPrefValue();
  SetPrefValue(setting);
}

IOSChromeScopedTestingBlockPopupsPref::
    ~IOSChromeScopedTestingBlockPopupsPref() {
  SetPrefValue(original_setting_);
}

// Gets the current value of the preference.
ContentSetting IOSChromeScopedTestingBlockPopupsPref::GetPrefValue() {
  ContentSetting popupSetting =
      ios::HostContentSettingsMapFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState())
          ->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, NULL);
  return popupSetting;
}

// Sets the preference to the given value.
void IOSChromeScopedTestingBlockPopupsPref::SetPrefValue(
    ContentSetting setting) {
  DCHECK(setting == CONTENT_SETTING_BLOCK || setting == CONTENT_SETTING_ALLOW);
  ios::ChromeBrowserState* state = chrome_test_util::GetOriginalBrowserState();
  ios::HostContentSettingsMapFactory::GetForBrowserState(state)
      ->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, setting);
}
