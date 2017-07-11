// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/test/browser_test.h"
#include "net/base/filename_util.h"

#if defined(URL_LOG_CONTENT_BROWSER_TEST)

#include "content/public/test/content_browser_test.h"  // nogncheck
#include "content/public/test/content_browser_test_utils.h"  // nogncheck
#define URL_LOG_BASE_BROWSER_TEST_CLASS content::ContentBrowserTest

#else

#include "chrome/test/base/in_process_browser_test.h"  // nogncheck
#define URL_LOG_BASE_BROWSER_TEST_CLASS InProcessBrowserTest

#endif

namespace {

class UrlLogBrowserTest : public URL_LOG_BASE_BROWSER_TEST_CLASS {
 public:
  UrlLogBrowserTest() = default;
  ~UrlLogBrowserTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    log_path_ = temp_dir_.GetPath().AppendASCII("UrlLogFile");

    PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir_);
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("components"));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("url_log"));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("test_data"));

    URL_LOG_BASE_BROWSER_TEST_CLASS::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchPath(url_log::kLogUrlLog, log_path_);
#if !defined(URL_LOG_CONTENT_BROWSER_TEST)
    command_line->AppendArg(getUrl(test_data_dir_, "test.html"));
#endif
  }

  std::string getUrl(const base::FilePath path, std::string child) {
    return net::FilePathToFileURL(
               test_data_dir_.Append(FILE_PATH_LITERAL(child)))
        .possibly_invalid_spec();
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath log_path_;
  base::FilePath test_data_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlLogBrowserTest);
};

IN_PROC_BROWSER_TEST_F(UrlLogBrowserTest, WriteUrlLog) {
#if defined(URL_LOG_CONTENT_BROWSER_TEST)
  // For content shell, we need an explicit navigation because
  // ContentBrowserTest adds --browser-test, which prevents loading the url
  // from the command line.
  content::NavigateToURL(shell(), GURL(getUrl(test_data_dir_, "test.html")));
#endif

  base::RunLoop().RunUntilIdle();

  base::ThreadRestrictions::ScopedAllowIO allow_io_for_log_path;
  ASSERT_TRUE(base::PathExists(log_path_));

  std::string input;
  ASSERT_TRUE(base::ReadFileToString(log_path_, &input));

  std::vector<std::string> lines = base::SplitString(
      input, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  ASSERT_TRUE(base::ContainsValue(lines, getUrl(test_data_dir_, "test.html")));
  ASSERT_TRUE(
      base::ContainsValue(lines, getUrl(test_data_dir_, "testharness.js")));
  ASSERT_TRUE(base::ContainsValue(lines, getUrl(test_data_dir_, "styles.css")));
  ASSERT_TRUE(base::ContainsValue(lines, getUrl(test_data_dir_, "child.html")));
  ASSERT_TRUE(base::ContainsValue(lines, getUrl(test_data_dir_, "child.js")));
}

}  // namespace
