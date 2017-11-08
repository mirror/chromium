// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/signin/chrome_signin_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "components/user_manager/user_type.h"
#include "content/public/browser/resource_context.h"
#include "content/public/test/browser_test.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"

namespace {

const GURL kGaiaUrl("https://accounts.google.com");
const char kChromeConnectedHeader[] = "X-Chrome-Connected";

}  // namespace

class ChromeSigninHelperTest : public InProcessBrowserTest {
 protected:
  ~ChromeSigninHelperTest() override {}

  ChromeSigninHelperTest() {}

  void CheckRequestHeader(net::URLRequest* url_request,
                          const char* header_name,
                          const std::string& expected_header_value) {
    bool expected_result = !expected_header_value.empty();
    std::string actual_header_value;
    EXPECT_EQ(expected_result, url_request->extra_request_headers().GetHeader(
                                   header_name, &actual_header_value))
        << header_name << ": " << actual_header_value;
    if (expected_result) {
      EXPECT_EQ(expected_header_value, actual_header_value);
    }
  }

  DISALLOW_COPY_AND_ASSIGN(ChromeSigninHelperTest);
};

// Mirror is enabled for child accounts.
IN_PROC_BROWSER_TEST_F(ChromeSigninHelperTest,
                       TestMirrorRequestChromeOsChildAccount) {
  // On Chrome OS this is false.
  ASSERT_FALSE(signin::IsAccountConsistencyMirrorEnabled());
  // Child user
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  user_manager::User* user = user_manager->GetActiveUser();
  ASSERT_TRUE(user);
  user->SetIsChild(true);

  Profile* profile = browser()->profile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetInteger(prefs::kIncognitoModeAvailability,
                    IncognitoModePrefs::DISABLED);
  prefs->SetString(prefs::kGoogleServicesUserAccountId, "123456789");

  ProfileIOData* profile_io =
      ProfileIOData::FromResourceContext(profile->GetResourceContext());
  std::unique_ptr<net::URLRequest> request =
      profile_io->GetMainRequestContext()->CreateRequest(
          kGaiaUrl, net::DEFAULT_PRIORITY, nullptr,
          TRAFFIC_ANNOTATION_FOR_TESTS);
  signin::FixAccountConsistencyRequestHeader(request.get(), GURL(), profile_io);

  ASSERT_EQ(7, signin::PROFILE_MODE_INCOGNITO_DISABLED |
                   signin::PROFILE_MODE_ADD_ACCOUNT_DISABLED |
                   signin::PROFILE_MODE_SWITCH_ACCOUNT_DISABLED);
  CheckRequestHeader(request.get(), kChromeConnectedHeader,
                     "mode=7,enable_account_consistency=true");
}

// Mirror is not enabled for non-child accounts.
IN_PROC_BROWSER_TEST_F(ChromeSigninHelperTest,
                       TestMirrorRequestChromeOsNotChildAccount) {
  // On Chrome OS this is false.
  ASSERT_FALSE(signin::IsAccountConsistencyMirrorEnabled());
  // Not a child user
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  user_manager::User* user = user_manager->GetActiveUser();
  ASSERT_TRUE(user);
  user->SetIsChild(false);

  Profile* profile = browser()->profile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetInteger(prefs::kIncognitoModeAvailability,
                    IncognitoModePrefs::ENABLED);
  prefs->SetString(prefs::kGoogleServicesUserAccountId, "123456789");

  ProfileIOData* profile_io =
      ProfileIOData::FromResourceContext(profile->GetResourceContext());
  std::unique_ptr<net::URLRequest> request =
      profile_io->GetMainRequestContext()->CreateRequest(
          kGaiaUrl, net::DEFAULT_PRIORITY, nullptr,
          TRAFFIC_ANNOTATION_FOR_TESTS);
  signin::FixAccountConsistencyRequestHeader(request.get(), GURL(), profile_io);

  CheckRequestHeader(request.get(), kChromeConnectedHeader, "");
}
