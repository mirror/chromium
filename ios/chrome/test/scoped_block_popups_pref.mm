// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/test/scoped_block_popups_pref.h"

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
//#import "ios/chrome/test/app/chrome_test_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ScopedBlockPopupsPref::ScopedBlockPopupsPref(
    ContentSetting setting,
    ios::ChromeBrowserState* browser_state)
    : original_setting_(GetPrefValue()), browser_state_(browser_state) {
  SetPrefValue(setting);
}

ScopedBlockPopupsPref::~ScopedBlockPopupsPref() {
  SetPrefValue(original_setting_);
}

// Gets the current value of the preference.
ContentSetting ScopedBlockPopupsPref::GetPrefValue() {
  ContentSetting popup_setting =
      ios::HostContentSettingsMapFactory::GetForBrowserState(browser_state_)
          ->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, NULL);
  return popup_setting;
}

// Sets the preference to the given value.
void ScopedBlockPopupsPref::SetPrefValue(ContentSetting setting) {
  DCHECK(setting == CONTENT_SETTING_BLOCK || setting == CONTENT_SETTING_ALLOW);
  ios::HostContentSettingsMapFactory::GetForBrowserState(browser_state_)
      ->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, setting);
}
