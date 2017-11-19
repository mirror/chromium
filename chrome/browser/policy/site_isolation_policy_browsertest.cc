// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/policy_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {
// SiteIsolationPolicyBrowserTests differ in the setup of the browser. Since the
// setup is done before the actual test is run, we need to parameterize our
// tests outside of the actual test bodies. We use test variants for this,
// instead of the usual setup of mulitple tests.
enum class TestVariant { kNone, kCommandline, kPolicy };
}  // namespace

class SiteIsolationPolicyBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<TestVariant> {
 public:
  // TODO(palmer): Probably don't need this.
  void SetUpOnMainThread() override {
    // We need this, so we can request the test page from 'http://foo.com'.
    // (Which, unlike 127.0.0.1, is considered an insecure origin.)
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  // TODO(palmer): Probably don't need this either.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // We need to know the server port to know what to add to the command-line.
    // The port number changes with every test run. Thus, we start the server
    // here. And since all tests, not just the variant with the command-line,
    // need the embedded server, we unconditionally start it here.
    EXPECT_TRUE(embedded_test_server()->Start());

    if (GetParam() != TestVariant::kCommandline)
      return;

    command_line->AppendSwitchASCII("unsafely-treat-insecure-origin-as-secure",
                                    BaseURL());
  }

  void SetUpInProcessBrowserTestFixture() override {
    if (GetParam() != TestVariant::kPolicy)
      return;

    // We setup the policy here, because the policy must be 'live' before
    // the renderer is created, since the value for this policy is passed
    // to the renderer via a command-line. Setting the policy in the test
    // itself or in SetUpOnMainThread works for update-able policies, but
    // is too late for this one.
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    policy::PolicyMap values;
    values.Set(policy::key::kUnsafelyTreatInsecureOriginsAsSecure,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_CLOUD,
               base::MakeUnique<base::Value>(BaseURL()), nullptr);
    provider_.UpdateChromePolicy(values);
  }

  bool ExpectSecureContext() { return GetParam() != TestVariant::kNone; }

  std::string BaseURL() {
    return embedded_test_server()->GetURL("foo.com", "/").spec();
  }

 private:
  policy::MockConfigurationPolicyProvider provider_;
};

INSTANTIATE_TEST_CASE_P(SiteIsolationPolicyBrowserTest,
                        SiteIsolationPolicyBrowserTest,
                        testing::Values(TestVariant::kNone,
                                        TestVariant::kCommandline,
                                        TestVariant::kPolicy));

IN_PROC_BROWSER_TEST_P(SiteIsolationPolicyBrowserTest, Simple) {
  GURL url = embedded_test_server()->GetURL(
      "foo.com", "/insecure_origin_browsertest.html");
  ui_test_utils::NavigateToURL(browser(), url);

  base::string16 secure(base::ASCIIToUTF16("secure"));
  base::string16 insecure(base::ASCIIToUTF16("insecure"));

  content::TitleWatcher title_watcher(
      browser()->tab_strip_model()->GetActiveWebContents(), secure);
  title_watcher.AlsoWaitForTitle(insecure);

  EXPECT_EQ(title_watcher.WaitAndGetTitle(),
            ExpectSecureContext() ? secure : insecure);
}
