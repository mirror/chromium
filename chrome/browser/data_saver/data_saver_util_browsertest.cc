// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_saver/data_saver_util.h"

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/resource_context.h"
#include "content/public/test/browser_test_base.h"
#include "content/public/test/browser_test_utils.h"

namespace chrome {

namespace {

const GURL kHttpUrl = GURL("http://www.example.test/");
const GURL kHttpsUrl = GURL("https://www.example.test/");

void CheckWouldLikelyBeFetchedViaDataSaverIO(ProfileIOData* profile_io_data,
                                             bool data_saver_enabled) {
  EXPECT_EQ(data_saver_enabled,
            WouldLikelyBeFetchedViaDataSaverIO(profile_io_data, kHttpUrl));
  EXPECT_FALSE(WouldLikelyBeFetchedViaDataSaverIO(profile_io_data, kHttpsUrl));
}

class DataSaverUtilBrowserTest : public InProcessBrowserTest {
 protected:
  void EnableDataSaver(bool enabled) {
    PrefService* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(prefs::kDataSaverEnabled, enabled);
    // Give the setting notification a chance to propagate.
    content::RunAllPendingInMessageLoop();
  }

  void RunOnIOThreadBlocking(base::OnceClosure task) {
    base::RunLoop run_loop;
    content::BrowserThread::PostTaskAndReply(content::BrowserThread::IO,
                                             FROM_HERE, std::move(task),
                                             run_loop.QuitClosure());
    run_loop.Run();
  }

  ProfileIOData* profile_io_data() {
    content::BrowserContext* brower_context = browser()->profile();
    content::ResourceContext* resource_context =
        brower_context->GetResourceContext();
    return ProfileIOData::FromResourceContext(resource_context);
  }
};

IN_PROC_BROWSER_TEST_F(DataSaverUtilBrowserTest, DataSaverEnabledIO) {
  EnableDataSaver(true);
  RunOnIOThreadBlocking(base::BindOnce(&CheckWouldLikelyBeFetchedViaDataSaverIO,
                                       base::Unretained(profile_io_data()),
                                       true));
}

IN_PROC_BROWSER_TEST_F(DataSaverUtilBrowserTest, DataSaverDisabledIO) {
  EnableDataSaver(false);
  RunOnIOThreadBlocking(base::BindOnce(&CheckWouldLikelyBeFetchedViaDataSaverIO,
                                       base::Unretained(profile_io_data()),
                                       false));
}

}  // namespace

}  // namespace chrome
