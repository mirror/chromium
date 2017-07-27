// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/strings/string_util.h"
#include "chrome/browser/chromeos/fileapi/recent_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace {

class RecentUtilTest : public testing::Test {
 public:
  RecentUtilTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}

 protected:
  void SetUp() override { ASSERT_TRUE(testing_profile_manager_.SetUp()); }

  void CallParseRecentUrl(const storage::FileSystemURL& url,
                          RecentContext* context,
                          base::FilePath* extra_path) {
    base::RunLoop run_loop;
    ParseRecentUrlOnIOThread(
        nullptr /* file_system_context */, url,
        base::BindOnce(&RecentUtilTest::OnParseRecentUrl,
                       base::Unretained(this), base::Unretained(context),
                       base::Unretained(extra_path), run_loop.QuitClosure()));
    run_loop.Run();
  }

  void OnParseRecentUrl(RecentContext* out_context,
                        base::FilePath* out_extra_path,
                        const base::Closure& quit_closure,
                        RecentContext context,
                        const base::FilePath& extra_path) {
    *out_context = std::move(context);
    *out_extra_path = extra_path;
    quit_closure.Run();
  }

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager testing_profile_manager_;
};

TEST_F(RecentUtilTest, GetRecentMountPointPath) {
  Profile* profile = testing_profile_manager_.CreateTestingProfile("me");
  base::FilePath path = GetRecentMountPointPath(profile);
  EXPECT_TRUE(base::StartsWith(path.value(), "/special/recent-",
                               base::CompareCase::SENSITIVE));
}

TEST_F(RecentUtilTest, GetRecentMountPointName) {
  Profile* profile = testing_profile_manager_.CreateTestingProfile("me");
  std::string name = GetRecentMountPointName(profile);
  EXPECT_TRUE(base::StartsWith(name, "recent-", base::CompareCase::SENSITIVE));
}

TEST_F(RecentUtilTest, ParseRecentUrlOnIOThread) {
  Profile* profile = testing_profile_manager_.CreateTestingProfile("me");
  base::FilePath path =
      GetRecentMountPointPath(profile).AppendASCII("some_path");
  storage::FileSystemURL url = storage::FileSystemURL::CreateForTest(
      GURL("chrome-extension://origin"), storage::kFileSystemTypeRecent, path);

  RecentContext context;
  base::FilePath extra_path;
  CallParseRecentUrl(url, &context, &extra_path);

  EXPECT_TRUE(context.is_valid());
  EXPECT_EQ(profile, context.profile());
  EXPECT_EQ(GURL("chrome-extension://origin"), context.origin());
  EXPECT_EQ("some_path", extra_path.value());
}

TEST_F(RecentUtilTest, ParseRecentUrlOnIOThread_NotRecentUrl) {
  Profile* profile = testing_profile_manager_.CreateTestingProfile("me");
  base::FilePath path =
      GetRecentMountPointPath(profile).AppendASCII("some_path");
  storage::FileSystemURL url = storage::FileSystemURL::CreateForTest(
      GURL("chrome-extension://origin"), storage::kFileSystemTypeNativeLocal,
      path);

  RecentContext context;
  base::FilePath extra_path;
  CallParseRecentUrl(url, &context, &extra_path);

  EXPECT_FALSE(context.is_valid());
  EXPECT_TRUE(extra_path.empty());
}

}  // namespace
}  // namespace chromeos
