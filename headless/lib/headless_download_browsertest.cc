// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/byte_stream.h"
#include "content/browser/download/download_file_factory.h"
#include "content/browser/download/download_file_impl.h"
#include "content/browser/download/download_item_impl.h"
#include "content/browser/download/download_manager_impl.h"
#include "content/browser/download/download_task_runner.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_utils.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_download_manager_delegate.h"
#include "headless/public/headless_browser.h"
#include "headless/test/headless_browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/test/url_request/url_request_slow_download_job.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using namespace content;

namespace net {
class NetLogWithSource;
}

namespace headless {

namespace {

const std::string kOriginOne = "one.example";
const std::string kOriginTwo = "two.example";

DownloadManagerImpl* DownloadManagerForBrowser(HeadlessBrowser* browser) {
  // We're in a content_browsertest; we know that the DownloadManager
  // is a DownloadManagerImpl.
  HeadlessBrowserContextImpl* ctx =
      HeadlessBrowserContextImpl::From(browser->GetDefaultBrowserContext());

  return static_cast<DownloadManagerImpl*>(
      BrowserContext::GetDownloadManager(ctx));
}

class CountingDownloadFile : public DownloadFileImpl {
 public:
  CountingDownloadFile(std::unique_ptr<DownloadSaveInfo> save_info,
                       const base::FilePath& default_downloads_directory,
                       std::unique_ptr<ByteStreamReader> stream,
                       const net::NetLogWithSource& net_log,
                       base::WeakPtr<DownloadDestinationObserver> observer)
      : DownloadFileImpl(std::move(save_info),
                         default_downloads_directory,
                         std::move(stream),
                         net_log,
                         observer) {}

  ~CountingDownloadFile() override {
    DCHECK(content::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
    active_files_--;
  }

  void Initialize(const InitializeCallback& callback,
                  const CancelRequestCallback& cancel_request_callback,
                  const DownloadItem::ReceivedSlices& received_slices,
                  bool is_parallelizable) override {
    DCHECK(content::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
    active_files_++;
    DownloadFileImpl::Initialize(callback, cancel_request_callback,
                                 received_slices, is_parallelizable);
  }

  static void GetNumberActiveFiles(int* result) {
    DCHECK(content::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
    *result = active_files_;
  }

  // Can be called on any thread, and will block (running message loop)
  // until data is returned.
  static int GetNumberActiveFilesFromFileThread() {
    int result = -1;
    content::GetDownloadTaskRunner()->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&CountingDownloadFile::GetNumberActiveFiles, &result),
        base::MessageLoop::current()->QuitWhenIdleClosure());
    base::RunLoop().Run();
    DCHECK_NE(-1, result);
    return result;
  }

 private:
  static int active_files_;
};

int CountingDownloadFile::active_files_ = 0;

class CountingDownloadFileFactory : public DownloadFileFactory {
 public:
  CountingDownloadFileFactory() {}
  ~CountingDownloadFileFactory() override {}

  // DownloadFileFactory interface.
  DownloadFile* CreateFile(
      std::unique_ptr<DownloadSaveInfo> save_info,
      const base::FilePath& default_downloads_directory,
      std::unique_ptr<ByteStreamReader> stream,
      const net::NetLogWithSource& net_log,
      base::WeakPtr<DownloadDestinationObserver> observer) override {
    return new CountingDownloadFile(std::move(save_info),
                                    default_downloads_directory,
                                    std::move(stream), net_log, observer);
  }
};

// Get the next created download.
class DownloadCreateObserver : DownloadManager::Observer {
 public:
  DownloadCreateObserver(DownloadManager* manager)
      : manager_(manager), item_(nullptr) {
    manager_->AddObserver(this);
  }

  ~DownloadCreateObserver() override {
    if (manager_)
      manager_->RemoveObserver(this);
    manager_ = nullptr;
  }

  void ManagerGoingDown(DownloadManager* manager) override {
    DCHECK_EQ(manager_, manager);
    manager_->RemoveObserver(this);
    manager_ = nullptr;
  }

  void OnDownloadCreated(DownloadManager* manager,
                         DownloadItem* download) override {
    if (!item_)
      item_ = download;

    if (!completion_closure_.is_null())
      base::ResetAndReturn(&completion_closure_).Run();
  }

  DownloadItem* WaitForFinished() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    if (!item_) {
      base::RunLoop run_loop;
      completion_closure_ = run_loop.QuitClosure();
      run_loop.Run();
    }
    return item_;
  }

 private:
  DownloadManager* manager_;
  DownloadItem* item_;
  base::Closure completion_closure_;
};

bool IsDownloadInState(DownloadItem::DownloadState state, DownloadItem* item) {
  return item->GetState() == state;
}

class HeadlessDownloadContentTest : public HeadlessBrowserTest {
 protected:
  std::unique_ptr<HeadlessDownloadManagerDelegate> test_delegate_;

  void SetUpOnMainThread() override {
    base::ThreadRestrictions::SetIOAllowed(true);
    ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());

    headless::HeadlessBrowserContext::Builder context_builder =
        browser()->CreateBrowserContextBuilder();
    headless::HeadlessBrowserContext* browser_context = context_builder.Build();
    browser()->SetDefaultBrowserContext(browser_context);

    test_delegate_.reset(new HeadlessDownloadManagerDelegate());
    test_delegate_->SetDownloadDirectory(downloads_directory_.GetPath());
    DownloadManager* manager = DownloadManagerForBrowser(browser());
    manager->GetDelegate()->Shutdown();
    manager->SetDelegate(test_delegate_.get());
    test_delegate_->SetDownloadManager(manager);

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&net::URLRequestSlowDownloadJob::AddUrlHandler));
    base::FilePath mock_base(GetTestFilePath("download", ""));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&net::URLRequestMockHTTPJob::AddUrlHandlers, mock_base));
    ASSERT_TRUE(embedded_test_server()->Start());
    const std::string real_host =
        embedded_test_server()->host_port_pair().host();
    host_resolver()->AddRule(kOriginOne, real_host);
    host_resolver()->AddRule(kOriginTwo, real_host);
  }

  // Create a DownloadTestObserverTerminal that will wait for the
  // specified number of downloads to finish.
  DownloadTestObserver* CreateWaiter(HeadlessBrowser* browser,
                                     int num_downloads) {
    DownloadManager* download_manager = DownloadManagerForBrowser(browser);
    return new DownloadTestObserverTerminal(
        download_manager, num_downloads,
        DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_FAIL);
  }

  void WaitForCompletion(DownloadItem* download) {
    DownloadUpdatedObserver(
        download, base::Bind(&IsDownloadInState, DownloadItem::COMPLETE))
        .WaitForEvent();
  }

  // Note: Cannot be used with other alternative DownloadFileFactorys
  void SetupEnsureNoPendingDownloads() {
    DownloadManagerForBrowser(browser())->SetDownloadFileFactoryForTesting(
        std::unique_ptr<DownloadFileFactory>(
            new CountingDownloadFileFactory()));
  }

  bool EnsureNoPendingDownloads() {
    bool result = true;
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&EnsureNoPendingDownloadJobsOnIO, &result));
    base::RunLoop().Run();
    return result &&
           (CountingDownloadFile::GetNumberActiveFilesFromFileThread() == 0);
  }

  HeadlessWebContents* CreateWebContentsForURL(HeadlessBrowser* browser,
                                               const GURL& url) {
    return browser->GetDefaultBrowserContext()
        ->CreateWebContentsBuilder()
        .SetInitialURL(url)
        .Build();
  }

  // Checks that |path| is has |file_size| bytes, and matches the |value|
  // string.
  bool VerifyFile(const base::FilePath& path,
                  const std::string& value,
                  const int64_t file_size) {
    std::string file_contents;

    {
      base::ThreadRestrictions::ScopedAllowIO allow_io_during_test_verification;
      bool read = base::ReadFileToString(path, &file_contents);
      EXPECT_TRUE(read) << "Failed reading file: " << path.value() << std::endl;
      if (!read)
        return false;  // Couldn't read the file.
    }

    // Note: we don't handle really large files (more than size_t can hold)
    // so we will fail in that case.
    size_t expected_size = static_cast<size_t>(file_size);

    // Check the size.
    EXPECT_EQ(expected_size, file_contents.size());
    if (expected_size != file_contents.size())
      return false;

    // Check the contents.
    EXPECT_EQ(value, file_contents);
    if (memcmp(file_contents.c_str(), value.c_str(), expected_size) != 0)
      return false;

    return true;
  }

  // Start a download and return the item.
  DownloadItem* StartDownloadAndReturnItem(HeadlessBrowser* browser, GURL url) {
    std::unique_ptr<DownloadCreateObserver> observer(
        new DownloadCreateObserver(DownloadManagerForBrowser(browser)));

    CreateWebContentsForURL(browser, url);
    return observer->WaitForFinished();
  }

 private:
  static void EnsureNoPendingDownloadJobsOnIO(bool* result) {
    if (net::URLRequestSlowDownloadJob::NumberOutstandingRequests())
      *result = false;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::MessageLoop::current()->QuitWhenIdleClosure());
  }

  // Location of the downloads directory for these tests
  base::ScopedTempDir downloads_directory_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(HeadlessDownloadContentTest, DownloadCancelled) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  test_delegate_->SetDownloadBehavior(
      HeadlessDownloadManagerDelegate::DownloadBehavior::ALLOW);

  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download = StartDownloadAndReturnItem(
      browser(), GURL(net::URLRequestSlowDownloadJob::kUnknownSizeUrl));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download->GetState());

  // Cancel the download and wait for download system quiesce.
  download->Cancel(true);
  scoped_refptr<DownloadTestFlushObserver> flush_observer(
      new DownloadTestFlushObserver(DownloadManagerForBrowser(browser())));
  flush_observer->WaitForFlush();

  // Get the important info from other threads and check it.
  EXPECT_TRUE(EnsureNoPendingDownloads());
}

// Check that downloading a single file works.
IN_PROC_BROWSER_TEST_F(HeadlessDownloadContentTest, SingleDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  test_delegate_->SetDownloadBehavior(
      HeadlessDownloadManagerDelegate::DownloadBehavior::ALLOW);

  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download1 = StartDownloadAndReturnItem(
      browser(),
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download1->GetState());

  WaitForCompletion(download1);
  ASSERT_EQ(DownloadItem::COMPLETE, download1->GetState());
}

// Check that downloading a single file works.
IN_PROC_BROWSER_TEST_F(HeadlessDownloadContentTest, DeniedDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  test_delegate_->SetDownloadBehavior(
      HeadlessDownloadManagerDelegate::DownloadBehavior::DENY);

  // Create a download, wait and confirm in the expected state.
  DownloadItem* download1 = StartDownloadAndReturnItem(
      browser(),
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  EnsureNoPendingDownloads();
  ASSERT_EQ(DownloadItem::CANCELLED, download1->GetState());
}

// Check that downloading a single file works.
IN_PROC_BROWSER_TEST_F(HeadlessDownloadContentTest, DeniedDefaultDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();

  // Create a download, wait and confirm in the expected state.
  DownloadItem* download1 = StartDownloadAndReturnItem(
      browser(),
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  EnsureNoPendingDownloads();
  ASSERT_EQ(DownloadItem::CANCELLED, download1->GetState());
}

// Check that downloading multiple (in this case, 2) files does not result in
// corrupted files.
IN_PROC_BROWSER_TEST_F(HeadlessDownloadContentTest, MultiDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  test_delegate_->SetDownloadBehavior(
      HeadlessDownloadManagerDelegate::DownloadBehavior::ALLOW);

  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download1 = StartDownloadAndReturnItem(
      browser(), GURL(net::URLRequestSlowDownloadJob::kUnknownSizeUrl));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download1->GetState());

  // Start the second download and wait until it's done.
  GURL url(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib"));
  DownloadItem* download2 = StartDownloadAndReturnItem(browser(), url);
  WaitForCompletion(download2);

  ASSERT_EQ(DownloadItem::IN_PROGRESS, download1->GetState());
  ASSERT_EQ(DownloadItem::COMPLETE, download2->GetState());

  // Allow the first request to finish.
  std::unique_ptr<DownloadTestObserver> observer2(CreateWaiter(browser(), 1));
  CreateWebContentsForURL(
      browser(), GURL(net::URLRequestSlowDownloadJob::kFinishDownloadUrl));
  observer2->WaitForFinished();  // Wait for the third request.
  EXPECT_EQ(1u, observer2->NumDownloadsSeenInState(DownloadItem::COMPLETE));

  // Get the important info from other threads and check it.
  EXPECT_TRUE(EnsureNoPendingDownloads());

  // The |DownloadItem|s should now be done and have the final file names.
  // Verify that the files have the expected data and size.
  // |file1| should be full of '*'s, and |file2| should be the same as the
  // source file.
  base::FilePath file1(download1->GetTargetFilePath());
  size_t file_size1 = net::URLRequestSlowDownloadJob::kFirstDownloadSize +
                      net::URLRequestSlowDownloadJob::kSecondDownloadSize;
  std::string expected_contents(file_size1, '*');
  ASSERT_TRUE(VerifyFile(file1, expected_contents, file_size1));

  base::FilePath file2(download2->GetTargetFilePath());
  ASSERT_TRUE(base::ContentsEqual(
      file2, GetTestFilePath("download", "download-test.lib")));
}

}  // namespace headless
