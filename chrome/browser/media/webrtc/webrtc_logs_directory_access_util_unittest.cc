// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_logs_directory_access_util.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/granted_file_entry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc_logs_directory_access_util {

class WebRtcLogsDirectoryAccessUtilTest : public testing::Test {
 public:
  WebRtcLogsDirectoryAccessUtilTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
    base::FilePath test_webrtc_logs_path;

    TestingProfile::Builder profile_builder;
    profile_builder.SetPath(test_webrtc_logs_path);
    profile_ = profile_builder.Build();
  }

  Profile* profile() { return profile_.get(); }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;

 private:
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(WebRtcLogsDirectoryAccessUtilTest, GetLogsDirectoryEntryReturnsEntry) {
  extensions::GrantedFileEntry entry = GetLogsDirectoryEntry(profile(), 0);
  EXPECT_TRUE(entry.filesystem_id.size() > 0);
  EXPECT_TRUE(entry.registered_name.size() > 0);
}

}  // namespace webrtc_logs_directory_access_util
