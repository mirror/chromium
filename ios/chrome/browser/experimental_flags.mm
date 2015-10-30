// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file can be empty. Its purpose is to contain the relatively short lived
// definitions required for experimental flags.

#include "ios/chrome/browser/experimental_flags.h"

#import <Foundation/Foundation.h>

#include <string>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "components/enhanced_bookmarks/enhanced_bookmark_features.h"
#include "components/variations/variations_associated_data.h"
#include "ios/chrome/browser/chrome_switches.h"
#include "ios/web/public/web_view_creation_util.h"

namespace {
NSString* const kEnableAlertOnBackgroundUpload =
    @"EnableAlertsOnBackgroundUpload";
NSString* const kEnableBookmarkRefreshImageOnEachVisit =
    @"EnableBookmarkRefreshImageOnEachVisit";
NSString* const kEnableViewCopyPasswords = @"EnableViewCopyPasswords";

enum class WKWebViewEligibility {
  // UNSET indicates that no explicit call to set eligibility has been made,
  // nor has a default value been assumed due to checking eligibility.
  UNSET,
  ELIGIBLE,
  INELIGIBLE
};
WKWebViewEligibility g_wkwebview_trial_eligibility =
    WKWebViewEligibility::UNSET;
}  // namespace

namespace experimental_flags {

bool IsAlertOnBackgroundUploadEnabled() {
  return [[NSUserDefaults standardUserDefaults]
      boolForKey:kEnableAlertOnBackgroundUpload];
}

bool IsExternalURLBlockingEnabled() {
  std::string group_name =
      base::FieldTrialList::FindFullName("IOSBlockUnpromptedExternalURLs");

  // Check if the experimental flag is turned on.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(
          switches::kEnableIOSBlockUnpromptedExternalURLs)) {
    return true;
  } else if (command_line->HasSwitch(
                 switches::kDisableIOSBlockUnpromptedExternalURLs)) {
    return false;
  }

  // Check if the finch experiment is turned on.
  return !base::StartsWith(group_name, "Disabled",
                           base::CompareCase::INSENSITIVE_ASCII);
}

bool IsBookmarkCollectionEnabled() {
  return enhanced_bookmarks::IsEnhancedBookmarksEnabled();
}

bool IsBookmarkImageFetchingOnVisitEnabled() {
  if (!IsBookmarkCollectionEnabled())
    return false;

  NSUserDefaults* user_defaults = [NSUserDefaults standardUserDefaults];
  if ([user_defaults boolForKey:kEnableBookmarkRefreshImageOnEachVisit])
    return true;

  const char kFieldTrialName[] = "EnhancedBookmarks";
  std::string enable_fetching = variations::GetVariationParamValue(
      kFieldTrialName, "EnableImagesFetchingOnVisit");
  return !enable_fetching.empty();
}

void SetWKWebViewTrialEligibility(bool eligible) {
  // It's critical that the enabled state be consistently reported throughout
  // the life of the app, so ensure that this has not already been set.
  DCHECK(g_wkwebview_trial_eligibility == WKWebViewEligibility::UNSET);

  g_wkwebview_trial_eligibility = eligible ? WKWebViewEligibility::ELIGIBLE
                                           : WKWebViewEligibility::INELIGIBLE;
}

bool IsWKWebViewEnabled() {
  // If g_wkwebview_trial_eligibility hasn't been set, default it to
  // ineligibile. This ensures future calls to try to set it will DCHECK.
  if (g_wkwebview_trial_eligibility == WKWebViewEligibility::UNSET) {
    g_wkwebview_trial_eligibility = WKWebViewEligibility::INELIGIBLE;
  }

  // If WKWebView isn't supported, don't activate the experiment at all. This
  // avoids someone being slotted into the WKWebView bucket (and thus reporting
  // as WKWebView), but actually running UIWebView.
  if (!web::IsWKWebViewSupported())
    return false;

  // Check for a flag forcing a specific group.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  bool force_enable = command_line->HasSwitch(switches::kEnableIOSWKWebView);
  bool force_disable = command_line->HasSwitch(switches::kDisableIOSWKWebView);
  bool trial_overridden = force_enable || force_disable;

  // If the user isn't eligible for the trial (i.e., their state is such that
  // they should not be automatically selected for the group), and there's no
  // explicit override, don't check the group (again, to avoid having them
  // report as part of a group at all).
  if (g_wkwebview_trial_eligibility == WKWebViewEligibility::INELIGIBLE &&
      !trial_overridden)
    return false;

  // Now that it's been established that user is a candidate, set up the trial
  // by checking the group.
  std::string group_name =
      base::FieldTrialList::FindFullName("IOSUseWKWebView");

  // Check if the experimental flag is turned on.
  if (force_enable)
    return true;
  else if (force_disable)
    return false;

  // Check if the finch experiment is turned on.
  return base::StartsWith(group_name, "Enabled",
                          base::CompareCase::INSENSITIVE_ASCII);
}

bool AreKeyboardCommandsEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableKeyboardCommands);
}

bool IsViewCopyPasswordsEnabled() {
  NSString* viewCopyPasswordFlag = [[NSUserDefaults standardUserDefaults]
      objectForKey:kEnableViewCopyPasswords];
  if ([viewCopyPasswordFlag isEqualToString:@"Enabled"])
    return true;
  return false;
}

}  // namespace experimental_flags
