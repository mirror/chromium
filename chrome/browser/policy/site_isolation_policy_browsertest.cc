// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

class SiteIsolationPolicyBrowserTest : public InProcessBrowserTest {
 protected:
  SiteIsolationPolicyBrowserTest() {}

  struct Expectations {
    const char* url;
    bool isolated;
  };

  void CheckExpectations(Expectations* expectations, size_t count) {
    content::BrowserContext* context = browser()->profile();
    ChromeContentBrowserClient client;
    for (size_t i = 0; i < count; ++i) {
      EXPECT_EQ(
          expectations[i].isolated,
          client.ShouldUseProcessPerSite(context, GURL(expectations[i].url)));
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SiteIsolationPolicyBrowserTest);
};

IN_PROC_BROWSER_TEST_F(SiteIsolationPolicyBrowserTest, SitePerProcess) {
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kSitePerProcess, true);

  Expectations expectations[] = {
      {"https://foo.com/noodles.html", true},
      {"http://foo.com/", true},
      {"http://example.org/pumpkins.html", true},
  };
  CheckExpectations(expectations, arraysize(expectations));
}

IN_PROC_BROWSER_TEST_F(SiteIsolationPolicyBrowserTest, IsolateOrigins) {
  browser()->profile()->GetPrefs()->SetString(
      prefs::kIsolateOrigins, "https://example.org/,http://example.com");

  Expectations expectations[] = {
      {"https://foo.com/noodles.html", false},
      {"http://foo.com/", false},
      {"https://example.org/pumpkins.html", true},
      {"http://example.com/index.php", true},
  };
  CheckExpectations(expectations, arraysize(expectations));
}
