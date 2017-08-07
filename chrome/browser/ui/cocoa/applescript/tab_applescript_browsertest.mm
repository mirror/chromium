// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/applescript/bookmark_applescript_utils_test.h"
#import "chrome/browser/ui/cocoa/applescript/error_applescript.h"
#import "chrome/browser/ui/cocoa/applescript/tab_applescript.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

typedef InProcessBrowserTest TabAppleScriptTest;

namespace {

IN_PROC_BROWSER_TEST_F(TabAppleScriptTest, Creation) {
  Profile* profile = browser()->profile();
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  base::scoped_nsobject<TabAppleScript> tab_applescript(
      [[TabAppleScript alloc] initWithWebContents:nullptr profile:nullptr]);
  EXPECT_FALSE(tab_applescript);

  tab_applescript.reset(
      [[TabAppleScript alloc] initWithWebContents:nullptr profile:profile]);
  EXPECT_FALSE(tab_applescript);

  tab_applescript.reset([[TabAppleScript alloc] initWithWebContents:web_contents
                                                            profile:nullptr]);
  EXPECT_FALSE(tab_applescript);

  tab_applescript.reset([[TabAppleScript alloc] initWithWebContents:web_contents
                                                            profile:profile]);
  EXPECT_TRUE(tab_applescript);
}

// Test JavaScript execution.
IN_PROC_BROWSER_TEST_F(TabAppleScriptTest, ExecuteJavascript) {
  Profile* profile = browser()->profile();
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::scoped_nsobject<TabAppleScript> tab_applescript([[TabAppleScript alloc]
      initWithWebContents:web_contents
                  profile:profile]);

  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kAllowJavascriptAppleEvents, false);

  // If scripter enters invalid URL.
  base::scoped_nsobject<FakeScriptCommand> fakeScriptCommand(
      [[FakeScriptCommand alloc] init]);
  [tab_applescript handlesExecuteJavascriptScriptCommand:nil];
  EXPECT_EQ(AppleScript::ErrorCode::errJavaScriptUnsupported,
            [NSScriptCommand currentCommand].scriptErrorNumber);

  fakeScriptCommand.reset([[FakeScriptCommand alloc] init]);
  prefs->SetBoolean(prefs::kAllowJavascriptAppleEvents, true);
  [tab_applescript handlesExecuteJavascriptScriptCommand:nil];
  EXPECT_EQ(0, [NSScriptCommand currentCommand].scriptErrorNumber);
}

}  // namespace
