// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace url_log {

namespace {

const char kUrl[] = "file://dir/test.html";

class UrlLogTest : public testing::Test {
 public:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    log_path_ = temp_dir_.GetPath().AppendASCII("UrlLogFile");
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath log_path_;
};

TEST_F(UrlLogTest, WriteUrl) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(kLogUrlLog,
                                                           log_path_);
  UrlLog logger;
  logger.Log(GURL(kUrl));

  EXPECT_TRUE(base::PathExists(log_path_));

  std::string input;
  EXPECT_TRUE(base::ReadFileToString(log_path_, &input));

  base::TrimWhitespaceASCII(input, base::TRIM_ALL, &input);
  EXPECT_EQ(kUrl, input) << "Expect " << input << " to be " << kUrl;
}

TEST_F(UrlLogTest, SkipWritingUrl) {
  UrlLog logger;
  logger.Log(GURL(kUrl));
  EXPECT_FALSE(base::PathExists(log_path_));
}

}  // namespace

}  // namespace url_log
