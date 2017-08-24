// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/earl_grey/chrome_settings_matchers.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/settings/accounts_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/clear_browsing_data_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/import_data_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/settings_collection_view_controller.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::ButtonWithAccessibilityLabel;

namespace chrome_test_util {

id<GREYMatcher> SettingsView() {
  return grey_accessibilityID(kSettingsCollectionViewId);
}

id<GREYMatcher> ClearBrowsingDataCollectionView() {
  return grey_accessibilityID(kClearBrowsingDataCollectionViewId);
}

id<GREYMatcher> SignInMenuButton() {
  return grey_accessibilityID(kSettingsSignInCellId);
}

id<GREYMatcher> PrimarySignInButton() {
  return SignInMenuButton();
}

id<GREYMatcher> SecondarySignInButton() {
  return SignInMenuButton();
}

id<GREYMatcher> SettingsAccountButton() {
  return grey_accessibilityID(kSettingsAccountCellId);
}

id<GREYMatcher> SettingsAccountsCollectionView() {
  return grey_accessibilityID(kSettingsAccountsId);
}

id<GREYMatcher> SettingsImportDataImportButton() {
  return grey_accessibilityID(kImportDataImportCellId);
}

id<GREYMatcher> SettingsImportDataKeepSeparateButton() {
  return grey_accessibilityID(kImportDataKeepSeparateCellId);
}

id<GREYMatcher> AccountsSyncButton() {
  return grey_accessibilityID(kSettingsAccountsSyncCellId);
}

id<GREYMatcher> ContentSettingsButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_CONTENT_SETTINGS_TITLE);
}

id<GREYMatcher> SettingsMenuBackButton() {
  return grey_allOf(grey_accessibilityID(@"ic_arrow_back"),
                    grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
}

id<GREYMatcher> SettingsMenuPrivacyButton() {
  return ButtonWithAccessibilityLabelId(
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_PRIVACY);
}

id<GREYMatcher> SettingsMenuPasswordsButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_SAVE_PASSWORDS);
}

id<GREYMatcher> ClearBrowsingHistoryButton() {
  return grey_allOf(grey_accessibilityID(kClearBrowsingHistoryCellId),
                    grey_sufficientlyVisible(), nil);
}

id<GREYMatcher> ClearCookiesButton() {
  return grey_accessibilityID(kClearCookiesCellId);
}

id<GREYMatcher> ClearCacheButton() {
  return grey_allOf(grey_accessibilityID(kClearCacheCellId),
                    grey_sufficientlyVisible(), nil);
}

id<GREYMatcher> ClearSavedPasswordsButton() {
  return grey_allOf(grey_accessibilityID(kClearSavedPasswordsCellId),
                    grey_sufficientlyVisible(), nil);
}

id<GREYMatcher> ClearBrowsingDataButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_CLEAR_BUTTON);
}

id<GREYMatcher> ConfirmClearBrowsingDataButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_CONFIRM_CLEAR_BUTTON);
}

id<GREYMatcher> SendUsageDataButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_OPTIONS_SEND_USAGE_DATA);
}

id<GREYMatcher> ClearBrowsingDataCell() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_CLEAR_BROWSING_DATA_TITLE);
}

id<GREYMatcher> SearchEngineButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_SEARCH_ENGINE_SETTING_TITLE);
}

id<GREYMatcher> AutofillButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_AUTOFILL);
}

id<GREYMatcher> GoogleChromeButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_PRODUCT_NAME);
}

id<GREYMatcher> VoiceSearchButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_VOICE_SEARCH_SETTING_TITLE);
}

id<GREYMatcher> BandwidthPreloadWebpagesButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_OPTIONS_PRELOAD_WEBPAGES);
}

id<GREYMatcher> PrivacyHandoffButton() {
  return ButtonWithAccessibilityLabelId(
      IDS_IOS_OPTIONS_ENABLE_HANDOFF_TO_OTHER_DEVICES);
}

id<GREYMatcher> BlockPopupsButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_BLOCK_POPUPS);
}

id<GREYMatcher> TranslateSettingsButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_TRANSLATE_SETTING);
}

id<GREYMatcher> BandwidthSettingsButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_BANDWIDTH_MANAGEMENT_SETTINGS);
}

}  // namespace chrome_test_util
