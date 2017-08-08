// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/extensions/api/page_capture/page_capture_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/login/scoped_test_public_session_login_state.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "net/dns/mock_host_resolver.h"
#include "storage/browser/blob/scoped_file.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/download/download_prefs.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/ui/browser_commands.h"

#if defined(OS_CHROMEOS)
#include "chromeos/login/login_state.h"
#endif  // defined(OS_CHROMEOS)

using extensions::PageCaptureSaveAsMHTMLFunction;
using extensions::ScopedTestDialogAutoConfirm;

class ExtensionPageCaptureApiTest : public ExtensionApiTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, "--expose-gc");
  }
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }
};

class PageCaptureSaveAsMHTMLDelegate
    : public PageCaptureSaveAsMHTMLFunction::TestDelegate {
 public:
  PageCaptureSaveAsMHTMLDelegate() {
    PageCaptureSaveAsMHTMLFunction::SetTestDelegate(this);
  }

  virtual ~PageCaptureSaveAsMHTMLDelegate() {
    PageCaptureSaveAsMHTMLFunction::SetTestDelegate(NULL);
  }

  void OnTemporaryFileCreated(const base::FilePath& temp_file) override {
    temp_file_ = temp_file;
  }

  base::FilePath temp_file_;
};

IN_PROC_BROWSER_TEST_F(ExtensionPageCaptureApiTest, SaveAsMHTML) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  PageCaptureSaveAsMHTMLDelegate delegate;
  ASSERT_TRUE(RunExtensionTest("page_capture")) << message_;
  // Make sure the MHTML data gets written to the temporary file.
  ASSERT_FALSE(delegate.temp_file_.empty());
  // Flush the message loops to make sure the delete happens.
  content::RunAllBlockingPoolTasksUntilIdle();
  content::RunAllPendingInMessageLoop(content::BrowserThread::IO);
  // Make sure the temporary file is destroyed once the javascript side reads
  // the contents.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  ASSERT_FALSE(base::PathExists(delegate.temp_file_));
}


// Notes: This test has nothing to with page_capture, I've putting
// it here because I was already changing this file :)
IN_PROC_BROWSER_TEST_F(ExtensionPageCaptureApiTest,
                       PRE_TestScopedFileWithoutBlockingShutdown) {
  LOG(INFO) << "Run PRE_TestScopedFileWithoutBlockingShutdown";
  Profile* profile = browser()->profile();
  base::ThreadRestrictions::ScopedAllowIO scoped_allow_io;
  base::ScopedTempDir temp_download_dir;
  ASSERT_TRUE(temp_download_dir.CreateUniqueTempDir());
  base::FilePath download_path = temp_download_dir.GetPath();
  LOG(INFO) << "Setting download_path to " << download_path.value();
  DownloadPrefs::FromBrowserContext(profile)->SetDownloadPath(
      download_path);
  base::FilePath hello_file(
      temp_download_dir.GetPath().AppendASCII("hello.txt"));
  ASSERT_EQ(3,
            base::WriteFile(hello_file, "cat", 3));
  // IMPORTANT: Leak the temp dir so that the test teardown
  // doesn't delete the directory. This will let us test ScopedFile that wraps
  // hello_file.
  temp_download_dir.Take();  // Intentional leak. IMPORTANT.

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(),
          base::TaskPriority::BACKGROUND,
          // THE BUG IS HERE.
          // Must use TaskShutdownBehavior::BLOCK_SHUTDOWN.
          base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
          //base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  // T1: Run a task that takes a while.
  auto run_big_task = []() {
    LOG(INFO) << "BEG run_big_task";
    int j = 0;
    std::string s = "test";
    for (int64_t i = 0; i < (1LL<<27); ++i) {
      s = "testing";
      s += (i % 10) > 5 ? 'a' : 'b';
      j = j + 2;
    }
    LOG(INFO) << "End run_big_task";
  };
  task_runner->PostTask(
      FROM_HERE, base::BindOnce(run_big_task));
  storage::ScopedFile scoped_file(
      hello_file,
      storage::ScopedFile::DELETE_ON_SCOPE_OUT,
      task_runner);
  // T2: |scoped_file| going out of scope task.

  // |task_runner| is sequenced task runner and we post T1 before T2.
  // T1 will take longer and since |task_runner| doesn't block on shutdown, the
  // ScopedFile destruction will not happen.
  // Therefore |TestScopedFile| will still see "hello_file".
}

IN_PROC_BROWSER_TEST_F(ExtensionPageCaptureApiTest,
                       TestScopedFileWithoutBlockingShutdown) {
  LOG(INFO) << "Run ScopedFile";
  Profile* profile = browser()->profile();
  base::ThreadRestrictions::ScopedAllowIO scoped_allow_io;
  base::ScopedTempDir temp_download_dir;
  base::FilePath download_path =
      DownloadPrefs::FromBrowserContext(profile)->DownloadPath();
  LOG(INFO) << "download_path = " << download_path.value();
  EXPECT_TRUE(temp_download_dir.Set(download_path));
  base::FilePath hello_file(
      temp_download_dir.GetPath().AppendASCII("hello.txt"));

  // We should not have seen this file still, ScopedFile in
  // TestScopedFileWithoutBlockingShutdown should have deleted the file.
  // However since TestScopedFileWithoutBlockingShutdown didn't use
  // BLOCK_SHUTDOWN for its |task_runner|, |run_big_task| didn't let ScopedFile
  // destruction code have a chance to run.
  ASSERT_FALSE(base::PathExists(hello_file));

  //std::string hello_contents;
  //ASSERT_TRUE(base::ReadFileToString(hello_file, &hello_contents));
  //ASSERT_EQ(std::string("cat"), hello_contents);
}

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(ExtensionPageCaptureApiTest,
                       PublicSessionRequestAllowed) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  PageCaptureSaveAsMHTMLDelegate delegate;
  chromeos::ScopedTestPublicSessionLoginState login_state;
  // Resolve Permission dialog with Allow.
  ScopedTestDialogAutoConfirm auto_confirm(ScopedTestDialogAutoConfirm::ACCEPT);
  ASSERT_TRUE(RunExtensionTest("page_capture")) << message_;
  ASSERT_FALSE(delegate.temp_file_.empty());
  content::RunAllBlockingPoolTasksUntilIdle();
  content::RunAllPendingInMessageLoop(content::BrowserThread::IO);
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  ASSERT_FALSE(base::PathExists(delegate.temp_file_));
}

IN_PROC_BROWSER_TEST_F(ExtensionPageCaptureApiTest,
                       PublicSessionRequestDenied) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  chromeos::ScopedTestPublicSessionLoginState login_state;
  // Resolve Permission dialog with Deny.
  ScopedTestDialogAutoConfirm auto_confirm(ScopedTestDialogAutoConfirm::CANCEL);
  ASSERT_TRUE(RunExtensionTestWithArg("page_capture", "REQUEST_DENIED"))
      << message_;
}
#endif  // defined(OS_CHROMEOS)
