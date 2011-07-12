// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/browser/translate/translate_prefs.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/live_sync/live_preferences_sync_test.h"

// TCM ID - 7306186.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kHomePageIsNewTabPage) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
}

// TCM ID - 7260488.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, Race) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));

  ChangeStringPref(0, prefs::kHomePage, "http://www.google.com/0");
  ChangeStringPref(1, prefs::kHomePage, "http://www.google.com/1");
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));
}

// TCM ID - 3649278.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kPasswordManagerEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));

  ChangeBooleanPref(0, prefs::kPasswordManagerEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));
}

// TCM ID - 3699293.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kKeepEverythingSynced) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kKeepEverythingSynced));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncThemes));

  GetClient(0)->DisableSyncForDatatype(syncable::THEMES);
  ASSERT_FALSE(BooleanPrefMatches(prefs::kKeepEverythingSynced));
}

// TCM ID - 3661290.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, DisablePreferences) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncPreferences));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));

  GetClient(1)->DisableSyncForDatatype(syncable::PREFERENCES);
  ChangeBooleanPref(0, prefs::kPasswordManagerEnabled);
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_FALSE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));

  GetClient(1)->EnableSyncForDatatype(syncable::PREFERENCES);
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));
}

// TCM ID - 3664292.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, DisableSync) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncPreferences));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));

  GetClient(1)->DisableSyncForAllDatatypes();
  ChangeBooleanPref(0, prefs::kPasswordManagerEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitSyncCycleCompletion("Changed a preference."));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));

  ChangeBooleanPref(1, prefs::kShowHomeButton);
  ASSERT_FALSE(BooleanPrefMatches(prefs::kShowHomeButton));

  GetClient(1)->EnableSyncForAllDatatypes();
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPasswordManagerEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
}

// TCM ID - 3604297.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, SignInDialog) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncPreferences));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncBookmarks));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncThemes));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncExtensions));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSyncAutofill));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kKeepEverythingSynced));

  GetClient(0)->DisableSyncForDatatype(syncable::PREFERENCES);
  GetClient(1)->EnableSyncForDatatype(syncable::PREFERENCES);
  GetClient(0)->DisableSyncForDatatype(syncable::AUTOFILL);
  GetClient(1)->EnableSyncForDatatype(syncable::AUTOFILL);
  GetClient(0)->DisableSyncForDatatype(syncable::BOOKMARKS);
  GetClient(1)->EnableSyncForDatatype(syncable::BOOKMARKS);
  GetClient(0)->DisableSyncForDatatype(syncable::EXTENSIONS);
  GetClient(1)->EnableSyncForDatatype(syncable::EXTENSIONS);
  GetClient(0)->DisableSyncForDatatype(syncable::THEMES);
  GetClient(1)->EnableSyncForDatatype(syncable::THEMES);

  ASSERT_TRUE(AwaitQuiescence());

  ASSERT_FALSE(BooleanPrefMatches(prefs::kSyncPreferences));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kSyncBookmarks));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kSyncThemes));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kSyncExtensions));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kSyncAutofill));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kKeepEverythingSynced));
}

// TCM ID - 3666296.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kShowBookmarkBar) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowBookmarkBar));

  ChangeBooleanPref(0, prefs::kShowBookmarkBar);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowBookmarkBar));
}

// TCM ID - 3611311.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kCheckDefaultBrowser) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kCheckDefaultBrowser));

  ChangeBooleanPref(0, prefs::kCheckDefaultBrowser);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kCheckDefaultBrowser));
}

// TCM ID - 3628298.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kHomePage) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));

  ChangeStringPref(0, prefs::kHomePage, "http://news.google.com");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));
}

// TCM ID - 7297269.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kShowHomeButton) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));

  ChangeBooleanPref(0, prefs::kShowHomeButton);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
}

// TCM ID - 3710285.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kEnableTranslate) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  ChangeBooleanPref(0, prefs::kEnableTranslate);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));
}

// TCM ID - 3664293.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kAutofillEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kAutofillEnabled));

  ChangeBooleanPref(0, prefs::kAutofillEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kAutofillEnabled));
}

// TCM ID - 3632259.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kURLsToRestoreOnStartup) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));
  ASSERT_TRUE(ListPrefMatches(prefs::kURLsToRestoreOnStartup));

  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));

  ListValue urls;
  urls.Append(Value::CreateStringValue("http://www.google.com/"));
  urls.Append(Value::CreateStringValue("http://www.flickr.com/"));
  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 4);
  ChangeListPref(0, prefs::kURLsToRestoreOnStartup, urls);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));
  ASSERT_TRUE(ListPrefMatches(prefs::kURLsToRestoreOnStartup));
}

// TCM ID - 3684287.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kRestoreOnStartup) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));

  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 1);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));
}

// TCM ID - 3703314.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, Privacy) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kAlternateErrorPagesEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSearchSuggestEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kNetworkPredictionEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));

  ChangeBooleanPref(0, prefs::kAlternateErrorPagesEnabled);
  ChangeBooleanPref(0, prefs::kSearchSuggestEnabled);
  ChangeBooleanPref(0, prefs::kNetworkPredictionEnabled);
  ChangeBooleanPref(0, prefs::kSafeBrowsingEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kAlternateErrorPagesEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSearchSuggestEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kNetworkPredictionEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));
}

// TCM ID - 3649279.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, ClearData) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteBrowsingHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteDownloadHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCache));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCookies));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeletePasswords));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteFormData));

  ChangeBooleanPref(0, prefs::kDeleteBrowsingHistory);
  ChangeBooleanPref(0, prefs::kDeleteDownloadHistory);
  ChangeBooleanPref(0, prefs::kDeleteCache);
  ChangeBooleanPref(0, prefs::kDeleteCookies);
  ChangeBooleanPref(0, prefs::kDeletePasswords);
  ChangeBooleanPref(0, prefs::kDeleteFormData);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteBrowsingHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteDownloadHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCache));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCookies));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeletePasswords));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteFormData));
}

// TCM ID - 3686300.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kWebKitUsesUniversalDetector) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kWebKitUsesUniversalDetector));

  ChangeBooleanPref(0, prefs::kWebKitUsesUniversalDetector);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kWebKitUsesUniversalDetector));
}

// TCM ID - 3673298.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kDefaultCharset) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(StringPrefMatches(prefs::kDefaultCharset));

  ChangeStringPref(0, prefs::kDefaultCharset, "Thai");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(StringPrefMatches(prefs::kDefaultCharset));
}

// TCM ID - 3653296.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kBlockThirdPartyCookies) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kBlockThirdPartyCookies));

  ChangeBooleanPref(0, prefs::kBlockThirdPartyCookies);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kBlockThirdPartyCookies));
}

// TCM ID - 7297279.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kClearSiteDataOnExit) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kClearSiteDataOnExit));

  ChangeBooleanPref(0, prefs::kClearSiteDataOnExit);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kClearSiteDataOnExit));
}

// TCM ID - 7306184.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kSafeBrowsingEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));

  ChangeBooleanPref(0, prefs::kSafeBrowsingEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));
}

// TCM ID - 3624302.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kAutofillAuxiliaryProfilesEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kAutofillAuxiliaryProfilesEnabled));

  ChangeBooleanPref(0, prefs::kAutofillAuxiliaryProfilesEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // kAutofillAuxiliaryProfilesEnabled is only synced on Mac.
#if defined(OS_MACOSX)
  ASSERT_TRUE(BooleanPrefMatches(prefs::kAutofillAuxiliaryProfilesEnabled));
#else
  ASSERT_FALSE(BooleanPrefMatches(prefs::kAutofillAuxiliaryProfilesEnabled));
#endif  // OS_MACOSX
}

// TCM ID - 3717298.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kPromptForDownload) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPromptForDownload));

  ChangeBooleanPref(0, prefs::kPromptForDownload);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPromptForDownload));
}

// TCM ID - 3729263.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kPrefTranslateLanguageBlacklist) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  TranslatePrefs translate_client0_prefs(GetPrefs(0));
  TranslatePrefs translate_client1_prefs(GetPrefs(1));
  ASSERT_FALSE(translate_client0_prefs.IsLanguageBlacklisted("fr"));
  translate_client0_prefs.BlacklistLanguage("fr");
  ASSERT_TRUE(translate_client0_prefs.IsLanguageBlacklisted("fr"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(translate_client1_prefs.IsLanguageBlacklisted("fr"));

  translate_client0_prefs.RemoveLanguageFromBlacklist("fr");
  ASSERT_FALSE(translate_client0_prefs.IsLanguageBlacklisted("fr"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(translate_client1_prefs.IsLanguageBlacklisted("fr"));
}

// TCM ID - 7307195.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kPrefTranslateWhitelists) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  TranslatePrefs translate_client0_prefs(GetPrefs(0));
  TranslatePrefs translate_client1_prefs(GetPrefs(1));
  ASSERT_FALSE(translate_client0_prefs.IsLanguagePairWhitelisted("en", "bg"));
  translate_client0_prefs.WhitelistLanguagePair("en", "bg");
  ASSERT_TRUE(translate_client0_prefs.IsLanguagePairWhitelisted("en", "bg"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(translate_client1_prefs.IsLanguagePairWhitelisted("en", "bg"));

  translate_client0_prefs.RemoveLanguagePairFromWhitelist("en", "bg");
  ASSERT_FALSE(translate_client0_prefs.IsLanguagePairWhitelisted("en", "bg"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(translate_client1_prefs.IsLanguagePairWhitelisted("en", "bg"));
}

// TCM ID - 3625298.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kPrefTranslateSiteBlacklist) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  GURL url("http://www.google.com");
  std::string host(url.host());
  TranslatePrefs translate_client0_prefs(GetPrefs(0));
  TranslatePrefs translate_client1_prefs(GetPrefs(1));
  ASSERT_FALSE(translate_client0_prefs.IsSiteBlacklisted(host));
  translate_client0_prefs.BlacklistSite(host);
  ASSERT_TRUE(translate_client0_prefs.IsSiteBlacklisted(host));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(translate_client1_prefs.IsSiteBlacklisted(host));

  translate_client0_prefs.RemoveSiteFromBlacklist(host);
  ASSERT_FALSE(translate_client0_prefs.IsSiteBlacklisted(host));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(translate_client1_prefs.IsSiteBlacklisted(host));
}

// TCM ID - 6515252.
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kExtensionsUIDeveloperMode) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kExtensionsUIDeveloperMode));

  ChangeBooleanPref(0, prefs::kExtensionsUIDeveloperMode);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kExtensionsUIDeveloperMode));
}

// TCM ID - 7583816
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kAcceptLanguages) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));

  AppendStringPref(0, prefs::kAcceptLanguages, ",ar");
  AppendStringPref(1, prefs::kAcceptLanguages, ",fr");
  ASSERT_TRUE(AwaitQuiescence());
  // kAcceptLanguages is not synced on Mac.
#if !defined(OS_MACOSX)
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));
#else
  ASSERT_FALSE(StringPrefMatches(prefs::kAcceptLanguages));
#endif  // OS_MACOSX

  ChangeStringPref(0, prefs::kAcceptLanguages, "en-US");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
#if !defined(OS_MACOSX)
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));
#else
  ASSERT_FALSE(StringPrefMatches(prefs::kAcceptLanguages));
#endif  // OS_MACOSX

  ChangeStringPref(0, prefs::kAcceptLanguages, "ar,en-US");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
#if !defined(OS_MACOSX)
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));
#else
  ASSERT_FALSE(StringPrefMatches(prefs::kAcceptLanguages));
#endif  // OS_MACOSX
}

// TCM ID - 7590682
#if defined(TOOLKIT_USES_GTK)
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kUsesSystemTheme) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kUsesSystemTheme));

  ChangeBooleanPref(0, prefs::kUsesSystemTheme);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kUsesSystemTheme));
}
#endif  // TOOLKIT_USES_GTK

// TCM ID - 3636292
#if defined(TOOLKIT_USES_GTK)
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       kUseCustomChromeFrame) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kUseCustomChromeFrame));

  ChangeBooleanPref(0, prefs::kUseCustomChromeFrame);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kUseCustomChromeFrame));
}
#endif  // TOOLKIT_USES_GTK

// TCM ID - 6473347.
#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kTapToClickEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kTapToClickEnabled));

  ChangeBooleanPref(0, prefs::kTapToClickEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kTapToClickEnabled));

  ChangeBooleanPref(1, prefs::kTapToClickEnabled);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kTapToClickEnabled));
}
#endif  // OS_CHROMEOS

// TCM ID - 6458824.
#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest, kEnableScreenLock) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableScreenLock));

  ChangeBooleanPref(0, prefs::kEnableScreenLock);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableScreenLock));

  ChangeBooleanPref(1, prefs::kEnableScreenLock);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableScreenLock));
}
#endif  // OS_CHROMEOS

IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       SingleClientEnabledEncryption) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IsEncrypted(0));
  ASSERT_TRUE(IsEncrypted(1));
}

IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       SingleClientEnabledEncryptionAndChanged) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IsEncrypted(0));
  ASSERT_TRUE(IsEncrypted(1));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
}

IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       BothClientsEnabledEncryption) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(EnableEncryption(1));
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(IsEncrypted(0));
  ASSERT_TRUE(IsEncrypted(1));
}

IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       SingleClientEnabledEncryptionBothChanged) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));

  ASSERT_TRUE(EnableEncryption(0));
  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ChangeStringPref(1, prefs::kHomePage, "http://www.google.com/1");
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(IsEncrypted(0));
  ASSERT_TRUE(IsEncrypted(1));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));
}

IN_PROC_BROWSER_TEST_F(TwoClientLivePreferencesSyncTest,
                       SingleClientEnabledEncryptionAndChangedMultipleTimes) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IsEncrypted(0));
  ASSERT_TRUE(IsEncrypted(1));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
  ChangeBooleanPref(0, prefs::kShowHomeButton);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
}
