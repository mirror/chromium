// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/destination_file_system.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class DestinationFileSystemTest : public testing::Test {
 public:
  DestinationFileSystemTest() = default;
  ~DestinationFileSystemTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(base::CreateTemporaryFile(&temp_file_));
    temp_file_ = temp_file_.ReplaceExtension(FILE_PATH_LITERAL("csv"));
    destination_ = std::make_unique<DestinationFileSystem>(temp_file_);
  }

  void TearDown() override { ASSERT_TRUE(base::DeleteFile(temp_file_, false)); }

 protected:
  base::FilePath temp_file_;
  std::unique_ptr<DestinationFileSystem> destination_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DestinationFileSystemTest);
};

TEST_F(DestinationFileSystemTest, WriteToFile) {
#if defined(OS_WIN)
  const char kLineEnding[] = "\r\n";
#else
  const char kLineEnding[] = "\n";
#endif
  const std::string kData = base::StringPrintf(
      "name,url,username,password%s"
      "accounts.google.com,http://accounts.google.com/a/"
      "LoginAuth,test@gmail.com,test1%s",
      kLineEnding, kLineEnding);

  destination_->Write(kData);

  std::string actual_written;
  base::ReadFileToString(temp_file_, &actual_written);
  EXPECT_EQ(kData, actual_written);
}
