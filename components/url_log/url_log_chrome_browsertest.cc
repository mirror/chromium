// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"
#include "components/url_log/url_log_browsertest_base.h"

#include "chrome/test/base/in_process_browser_test.h"

namespace url_log {

class UrlLogChromeBrowserTest : public InProcessBrowserTest, public UrlLogBrowserTestBase {
 public:
  UrlLogChromeBrowserTest() {};
  ~UrlLogChromeBrowserTest() override {};

  void SetUp() override {
    UrlLogBrowserTestBase::SetUp();
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    UrlLogBrowserTestBase::SetUpCommandLine(command_line);
    command_line->AppendArg(GetUrl(test_data_dir_, "test.html").spec());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlLogChromeBrowserTest);
};

IN_PROC_BROWSER_TEST_F(UrlLogChromeBrowserTest, WriteUrlLog) {
  UrlLogBrowserTestBase::WriteUrlLogTest();
}

}  // namespace url_log

