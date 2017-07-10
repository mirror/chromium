// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_command_line.h"
#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace url_log {

namespace {

const char kUrl[] = "file://dir/test.html";

class UrlLogTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    log_path_ = temp_dir_.GetPath().AppendASCII("UrlLogFile");
    command_line_ = base::MakeUnique<base::test::ScopedCommandLine>();

    base::DefaultSingletonTraits<UrlLog>::Delete(UrlLog::GetInstance());
  }

  void TearDown() override {
    // Reset the command line before resetting the UrlLog instance.
    command_line_.reset();
    base::DefaultSingletonTraits<UrlLog>::Delete(UrlLog::GetInstance());
    base::DefaultSingletonTraits<UrlLog>::New();
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath log_path_;
  std::unique_ptr<base::test::ScopedCommandLine> command_line_;
};

TEST_F(UrlLogTest, WriteUrl) {
  command_line_->GetProcessCommandLine()->AppendSwitchPath(
      kLogUrlLog, log_path_);
  base::DefaultSingletonTraits<UrlLog>::New();

  UrlLog::AddEntry(GURL(kUrl));

  std::string input;
  ASSERT_TRUE(base::ReadFileToString(log_path_, &input));

  std::string expected = base::StringPrintf("%s\n", kUrl);
  ASSERT_EQ(expected, input);
}

TEST_F(UrlLogTest, SkipWritingUrl) {
  base::DefaultSingletonTraits<UrlLog>::New();

  UrlLog::AddEntry(GURL(kUrl));
  ASSERT_FALSE(base::PathExists(log_path_));
}

}  // namespace

}  // namespace url_log
