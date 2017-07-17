// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "components/url_log/url_log.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/base/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char* const kExpectedFiles[] = {"test.html", "testharness.js",
                                      "styles.css", "child.html", "child.js"};

class ShellNetworkDelegateUrlLogBrowserTest
    : public content::ContentBrowserTest {
 public:
  ShellNetworkDelegateUrlLogBrowserTest() {
    PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir_);
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("components"));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("url_log"));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("test_data"));
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    log_path_ = temp_dir_.GetPath().AppendASCII("UrlLogFile");
    content::ContentBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchPath(url_log::kLogUrlLog, log_path_);
  }

 private:
  base::ScopedTempDir temp_dir_;
  base::FilePath log_path_;
  base::FilePath test_data_dir_;

  DISALLOW_COPY_AND_ASSIGN(ShellNetworkDelegateUrlLogBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ShellNetworkDelegateUrlLogBrowserTest, WriteUrlLog) {
  content::NavigateToURL(
      shell(), net::FilePathToFileURL(test_data_dir_.AppendASCII("test.html")));
  base::RunLoop().RunUntilIdle();

  base::ThreadRestrictions::ScopedAllowIO allow_io_for_log_path;
  EXPECT_TRUE(base::PathExists(log_path_));

  std::string input;
  EXPECT_TRUE(base::ReadFileToString(log_path_, &input));

  std::vector<std::string> lines = base::SplitString(
      input, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  for (const std::string& filename : kExpectedFiles) {
    std::string full_path(
        net::FilePathToFileURL(test_data_dir_.AppendASCII(filename)).spec());
    EXPECT_TRUE(base::ContainsValue(lines, full_path))
        << "Expect " << full_path << " to be in:\n"
        << input;
  }
}

}  // namespace
