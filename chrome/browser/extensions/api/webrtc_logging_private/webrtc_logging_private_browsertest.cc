// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_browsertest_util.h"

class WebrtcLoggingPrivateApiBrowserTest
    : public extensions::PlatformAppBrowserTest {
 public:
  WebrtcLoggingPrivateApiBrowserTest() = default;
  ~WebrtcLoggingPrivateApiBrowserTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebrtcLoggingPrivateApiBrowserTest);
};

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiBrowserTest,
                       TestGetLogsDirectory) {
  ASSERT_TRUE(
      RunPlatformAppTest("api_test/webrtc_logging_private/"
                         "get_logs_directory"))
      << message_;
}
