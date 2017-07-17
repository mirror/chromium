// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/net/chrome_network_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/url_log/url_log.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/filename_util.h"
#include "net/base/network_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char* const kExpectedFiles[] = {"test.html", "testharness.js",
                                      "styles.css", "child.html", "child.js"};

#if defined(OS_CHROMEOS)

class ChromeNetworkDelegateBrowserTest : public InProcessBrowserTest {
 protected:
  ChromeNetworkDelegateBrowserTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    // Access to all files via file: scheme is allowed on browser
    // tests. Bring back the production behaviors.
    ChromeNetworkDelegate::EnableAccessToAllFilesForTesting(false);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNetworkDelegateBrowserTest);
};

// Ensure that access to a test file, that is not in an accessible location,
// via file: scheme is rejected with ERR_ACCESS_DENIED.
IN_PROC_BROWSER_TEST_F(ChromeNetworkDelegateBrowserTest, AccessToFile) {
  base::FilePath test_dir;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &test_dir));
  base::FilePath test_file = test_dir.AppendASCII("empty.html");
  ASSERT_FALSE(
      ChromeNetworkDelegate::IsAccessAllowed(test_file, base::FilePath()));

  GURL url = net::FilePathToFileURL(test_file);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver observer(web_contents);
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_EQ(net::ERR_ACCESS_DENIED, observer.last_net_error_code());
}

// Ensure that access to a symbolic link, that is in an accessible location,
// to a test file, that isn't, via file: scheme is rejected with
// ERR_ACCESS_DENIED.
IN_PROC_BROWSER_TEST_F(ChromeNetworkDelegateBrowserTest, AccessToSymlink) {
  base::FilePath test_dir;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &test_dir));
  base::FilePath test_file = test_dir.AppendASCII("empty.html");
  ASSERT_FALSE(
      ChromeNetworkDelegate::IsAccessAllowed(test_file, base::FilePath()));

  base::FilePath temp_dir;
  ASSERT_TRUE(PathService::Get(base::DIR_TEMP, &temp_dir));
  base::ScopedTempDir scoped_temp_dir;
  ASSERT_TRUE(scoped_temp_dir.CreateUniqueTempDirUnderPath(temp_dir));
  base::FilePath symlink = scoped_temp_dir.GetPath().AppendASCII("symlink");
  ASSERT_TRUE(base::CreateSymbolicLink(test_file, symlink));
  ASSERT_TRUE(
      ChromeNetworkDelegate::IsAccessAllowed(symlink, base::FilePath()));

  GURL url = net::FilePathToFileURL(symlink);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver observer(web_contents);
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_EQ(net::ERR_ACCESS_DENIED, observer.last_net_error_code());
}

#endif  // defined(OS_CHROMEOS)

class ChromeNetworkDelegateUrlLogBrowserTest : public InProcessBrowserTest {
 public:
  ChromeNetworkDelegateUrlLogBrowserTest() {
    PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir_);
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("components"));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("url_log"));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("test_data"));
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    log_path_ = temp_dir_.GetPath().AppendASCII("UrlLogFile");
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchPath(url_log::kLogUrlLog, log_path_);
    command_line->AppendArg(
        net::FilePathToFileURL(test_data_dir_.AppendASCII("test.html")).spec());
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath log_path_;
  base::FilePath test_data_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNetworkDelegateUrlLogBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ChromeNetworkDelegateUrlLogBrowserTest, WriteUrlLog) {
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
