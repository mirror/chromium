// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/stringprintf.h"
#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/live_sync/live_preferences_sync_test.h"

IN_PROC_BROWSER_TEST_F(MultipleClientLivePreferencesSyncTest, Sanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  for (int i = 0; i < num_clients(); ++i) {
    ListValue urls;
    urls.Append(Value::CreateStringValue(
        base::StringPrintf("http://www.google.com/%d", i)));
    ChangeListPref(i, prefs::kURLsToRestoreOnStartup, urls);
  }

  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(ListPrefMatches(prefs::kURLsToRestoreOnStartup));
}
