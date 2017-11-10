// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/media/webrtc/webrtc_log_list.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/testing_profile.h"

class WebrtcLoggingPrivateApiBrowserTest
    : public extensions::PlatformAppBrowserTest {
 public:
  WebrtcLoggingPrivateApiBrowserTest() = default;
  ~WebrtcLoggingPrivateApiBrowserTest() override = default;

  base::FilePath webrtc_logs_path() { return webrtc_logs_path_; }

 protected:
  bool SetUpUserDataDirectory() override {
    base::FilePath profile_dir;
    PathService::Get(chrome::DIR_USER_DATA, &profile_dir);
    profile_dir = profile_dir.AppendASCII(TestingProfile::kTestUserProfileDir);
    base::CreateDirectory(profile_dir);
    webrtc_logs_path_ =
        WebRtcLogList::GetWebRtcLogDirectoryForProfile(profile_dir);
    return true;
  }

  base::FilePath webrtc_logs_path_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebrtcLoggingPrivateApiBrowserTest);
};

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiBrowserTest,
                       TestGetLogsDirectoryCreatesWebRtcLogsDirectory) {
  ASSERT_FALSE(base::PathExists(webrtc_logs_path()));
  ASSERT_TRUE(RunPlatformAppTestWithArg(
      "api_test/webrtc_logging_private/get_logs_directory",
      "test_without_directory"))
      << message_;
  ASSERT_TRUE(base::PathExists(webrtc_logs_path()));
  ASSERT_TRUE(base::IsDirectoryEmpty(webrtc_logs_path()));
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiBrowserTest,
                       TestGetLogsDirectoryReadsFiles) {
  ASSERT_TRUE(base::CreateDirectory(webrtc_logs_path()));
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath test_file_path = webrtc_logs_path().AppendASCII("test.file");
    std::string contents = "test file contents";
    EXPECT_EQ(
        base::checked_cast<int>(contents.size()),
        base::WriteFile(test_file_path, contents.c_str(), contents.size()));
  }
  ASSERT_TRUE(RunPlatformAppTestWithArg(
      "api_test/webrtc_logging_private/get_logs_directory",
      "test_with_file_in_directory"))
      << message_;
}
