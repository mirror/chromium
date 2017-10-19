// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_content_browser_client.h"

#include "base/command_line.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

#if defined(OS_CHROMEOS)
#include "base/base_switches.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/common/process_type.h"
#endif

namespace content {

// Use a test class with SetUpCommandLine to ensure the flag is sent to the
// first renderer process.
class ChromeContentBrowserClientBrowserTest : public InProcessBrowserTest {
 public:
  ChromeContentBrowserClientBrowserTest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeContentBrowserClientBrowserTest);
};

// Test that a basic navigation works in --site-per-process mode.  This prevents
// regressions when that mode calls out into the ChromeContentBrowserClient,
// such as http://crbug.com/164223.
IN_PROC_BROWSER_TEST_F(ChromeContentBrowserClientBrowserTest,
                       SitePerProcessNavigation) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url(embedded_test_server()->GetURL("/title1.html"));

  ui_test_utils::NavigateToURL(browser(), url);
  NavigationEntry* entry = browser()
                               ->tab_strip_model()
                               ->GetWebContentsAt(0)
                               ->GetController()
                               .GetLastCommittedEntry();

  ASSERT_TRUE(entry != NULL);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_EQ(url, entry->GetVirtualURL());
}

#if defined(OS_CHROMEOS)

namespace {

void GetUtilityProcessPidsOnIOThread(std::vector<pid_t>* pids) {
  BrowserChildProcessHostIterator it(PROCESS_TYPE_UTILITY);
  for (; !it.Done(); ++it) {
    pid_t pid = it.GetData().handle;
    pids->push_back(pid);
  }
}

std::string ReadCmdLine(pid_t pid) {
  // Files in "/proc" are in-memory, so it's safe to do IO.
  base::ScopedAllowBlockingForTesting allow_io;
  base::FilePath cmdline_file =
      base::FilePath("/proc").Append(base::IntToString(pid)).Append("cmdline");
  std::string cmdline;
  base::ReadFileToString(cmdline_file, &cmdline);
  return cmdline;
}

// We don't seem to have a string utility or STL utility for this.
bool HasSubstring(base::StringPiece str, base::StringPiece sub) {
  return str.find(sub) != base::StringPiece::npos;
}

}  // namespace

using ChromeContentBrowserClientMashTest = InProcessBrowserTest;

// Verifies that mash service child processes do not use in-process breakpad
// crash dumping.
IN_PROC_BROWSER_TEST_F(ChromeContentBrowserClientMashTest, CrashReporter) {
  if (chromeos::GetAshConfig() != ash::Config::MASH)
    return;

  // Child process management lives on the IO thread.
  std::vector<pid_t> pids;
  base::RunLoop loop;
  BrowserThread::PostTaskAndReply(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&GetUtilityProcessPidsOnIOThread, &pids), loop.QuitClosure());
  loop.Run();
  // There's at least one utility process, for ash.
  ASSERT_GT(pids.size(), 0u);

  // Iterate through all utility processes looking for mash services.
  int mash_service_count = 0;
  for (pid_t pid : pids) {
    std::string cmdline = ReadCmdLine(pid);
    ASSERT_FALSE(cmdline.empty());
    // Subprocess command lines may have their null separators replaced with
    // spaces, which makes them hard to tokenize. Just search the whole string.
    if (HasSubstring(cmdline, switches::kMashServiceName)) {
      ++mash_service_count;
      // Mash services should not use in-process breakpad crash dumping.
      EXPECT_FALSE(HasSubstring(cmdline, switches::kEnableCrashReporter));
    }
  }
  // There's at least one mash service, for ash.
  EXPECT_GT(mash_service_count, 0);
}

#endif  // defined(OS_CHROMEOS)

}  // namespace content
