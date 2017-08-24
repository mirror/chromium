// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_EARL_GREY_CHROME_SETTINGS_MATCHERS_H_
#define IOS_CHROME_TEST_EARL_GREY_CHROME_SETTINGS_MATCHERS_H_

#include <string>

#import <EarlGrey/EarlGrey.h>

namespace chrome_test_util {

// Matcher for the settings collection view.
id<GREYMatcher> SettingsView();

// Returns matcher for the clear browsing data collection view.
id<GREYMatcher> ClearBrowsingDataCollectionView();

// Returns matcher for the signin button in the settings menu.
id<GREYMatcher> SignInMenuButton();

// Returns SignInMenuButton() as long as the sign-in promo is not enabled for
// UI tests. When the sign-in promo will be enabled, it will returns matcher for
// the secondary button in the sign-in promo view. This is "Sign in into Chrome"
// button for a cold state, or "Continue as John Doe" for a warm state.
id<GREYMatcher> PrimarySignInButton();

// Returns SignInMenuButton() as long as the sign-in promo is not enabled for
// UI tests. When the sign-in promo will be enabled, it will returns matcher for
// the secondary button in the sign-in promo view. This is
// "Not johndoe@example.com" button.
id<GREYMatcher> SecondarySignInButton();

// Returns matcher for the button for the currently signed in account in the
// settings menu.
id<GREYMatcher> SettingsAccountButton();

// Returns matcher for the accounts collection view.
id<GREYMatcher> SettingsAccountsCollectionView();

// Returns matcher for the Import Data cell in switch sync account view.
id<GREYMatcher> SettingsImportDataImportButton();

// Returns matcher for the Keep Data Separate cell in switch sync account view.
id<GREYMatcher> SettingsImportDataKeepSeparateButton();

// Returns matcher for the menu button to sync accounts.
id<GREYMatcher> AccountsSyncButton();

// Returns matcher for the Content Settings button on the main Settings screen.
id<GREYMatcher> ContentSettingsButton();

// Returns matcher for the back button on a settings menu.
id<GREYMatcher> SettingsMenuBackButton();

// Returns matcher for the Privacy cell on the main Settings screen.
id<GREYMatcher> SettingsMenuPrivacyButton();

// Returns matcher for the Save passwords cell on the main Settings screen.
id<GREYMatcher> SettingsMenuPasswordsButton();

// Matcher for the clear browsing history cell on the clear browsing data panel.
id<GREYMatcher> ClearBrowsingHistoryButton();

// Matcher for the clear cookies cell on the clear browsing data panel.
id<GREYMatcher> ClearCookiesButton();

// Matcher for the clear cache cell on the clear browsing data panel.
id<GREYMatcher> ClearCacheButton();

// Matcher for the clear saved passwords cell on the clear browsing data panel.
id<GREYMatcher> ClearSavedPasswordsButton();

// Matcher for the clear browsing data button on the clear browsing data panel.
id<GREYMatcher> ClearBrowsingDataButton();

// Matcher for the clear browsing data action sheet item.
id<GREYMatcher> ConfirmClearBrowsingDataButton();

// Matcher for the Send Usage Data cell on the Privacy screen.
id<GREYMatcher> SendUsageDataButton();

// Matcher for the Clear Browsing Data cell on the Privacy screen.
id<GREYMatcher> ClearBrowsingDataCell();

// Matcher for the Search Engine cell on the main Settings screen.
id<GREYMatcher> SearchEngineButton();

// Matcher for the Autofill Forms cell on the main Settings screen.
id<GREYMatcher> AutofillButton();

// Matcher for the Google Chrome cell on the main Settings screen.
id<GREYMatcher> GoogleChromeButton();

// Matcher for the Google Chrome cell on the main Settings screen.
id<GREYMatcher> VoiceSearchButton();

// Matcher for the Preload Webpages button on the bandwidth UI.
id<GREYMatcher> BandwidthPreloadWebpagesButton();

// Matcher for the Privacy Handoff button on the privacy UI.
id<GREYMatcher> PrivacyHandoffButton();

// Matcher for the Privacy Block Popups button on the privacy UI.
id<GREYMatcher> BlockPopupsButton();

// Matcher for the Privacy Translate Settings button on the privacy UI.
id<GREYMatcher> TranslateSettingsButton();

// Matcher for the Bandwidth Settings button on the main Settings screen.
id<GREYMatcher> BandwidthSettingsButton();

}  // namespace chrome_test_util

#endif  // IOS_CHROME_TEST_EARL_GREY_CHROME_SETTINGS_MATCHERS_H_
