// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"
#include "components/url_log/url_log_browsertest_base.h"

#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"

namespace url_log {

class UrlLogContentBrowserTest : public content::ContentBrowserTest,
                                 public UrlLogBrowserTestBase {
 public:
  UrlLogContentBrowserTest() {};

  void SetUp() override {
    UrlLogBrowserTestBase::SetUp();
    content::ContentBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    UrlLogBrowserTestBase::SetUpCommandLine(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlLogContentBrowserTest);
};

IN_PROC_BROWSER_TEST_F(UrlLogContentBrowserTest, WriteUrlLog) {
  content::NavigateToURL(shell(), GURL(GetUrl(test_data_dir_, "test.html")));
  UrlLogBrowserTestBase::WriteUrlLogTest();
}

}  // namespace url_log
