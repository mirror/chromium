// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/supports_user_data.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/download/content/factory/download_service_factory.h"
#include "components/download/public/download_metadata.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_download_request_handler.h"
#include "testing/gmock/include/gmock/gmock.h"

// TODO(xingliu): Use mock scheduler?
#include "chrome/browser/download/download_task_scheduler_impl.h"

using testing::NiceMock;

namespace {

const char kCreationData[] = "DownloadServiceTestCreationData";

// TODO(xingliu): Move this thing to test support.
class MockClient : public download::Client {
 public:
  MockClient() = default;
  ~MockClient() override = default;

  // Client implementation.
  MOCK_METHOD2(OnServiceInitialized,
               void(bool, const std::vector<download::DownloadMetaData>&));
  MOCK_METHOD0(OnServiceUnavailable, void());
  MOCK_METHOD3(
      OnDownloadStarted,
      ShouldDownload(const std::string&,
                     const std::vector<GURL>&,
                     const scoped_refptr<const net::HttpResponseHeaders>&));
  MOCK_METHOD2(OnDownloadUpdated, void(const std::string&, uint64_t));
  MOCK_METHOD2(OnDownloadFailed,
               void(const std::string&, download::Client::FailureReason));
  MOCK_METHOD2(OnDownloadSucceeded,
               void(const std::string&, const download::CompletionInfo&));
  MOCK_METHOD2(CanServiceRemoveDownloadedFile, bool(const std::string&, bool));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockClient);
};

content::DownloadManager* GetDownloadManager(Browser* browser) {
  DCHECK(browser);
  return content::BrowserContext::GetDownloadManager(browser->profile());
}

// Used to pass data to download service factory in test, and glue things to
// test fixture at the same time.
struct CreationData : public base::SupportsUserData::Data {
  base::FilePath storage_directory;
  std::unique_ptr<download::Client> client;
  CreationData() = default;
  ~CreationData() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(CreationData);
};

// Factory used to create download service in test.
std::unique_ptr<KeyedService> TestDownloadServiceFactory(
    content::BrowserContext* context) {
  CreationData* creation_data =
      static_cast<CreationData*>(context->GetUserData(kCreationData));

  // Set up the download service.
  auto client_map = base::MakeUnique<download::DownloadClientMap>();
  client_map->emplace(download::DownloadClient::TEST,
                      std::move(creation_data->client));

  auto storage_directory = creation_data->storage_directory;

  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});

  std::unique_ptr<download::TaskScheduler> task_scheduler;
  task_scheduler = base::MakeUnique<DownloadTaskSchedulerImpl>(context);

  return base::WrapUnique<download::DownloadService>(
      download::CreateDownloadService(std::move(client_map), context,
                                      storage_directory, background_task_runner,
                                      std::move(task_scheduler)));
}

// Test fixture to verify components/download API.
class DownloadServiceTest : public InProcessBrowserTest {
 public:
  DownloadServiceTest() {}

  void SetUpOnMainThread() override {
    // Setup the directory and download manager.
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      ASSERT_TRUE(storage_directory_.CreateUniqueTempDir());
    }
    Profile* profile = browser()->profile();
    profile->GetPrefs()->SetBoolean(prefs::kPromptForDownload, false);
    content::DownloadManager* manager = GetDownloadManager(browser());
    DownloadPrefs::FromDownloadManager(manager)->ResetAutoOpen();

    // Setup the service and the client.
    auto client = base::MakeUnique<NiceMock<MockClient>>();
    client_ = client.get();
    auto creation_data = base::MakeUnique<CreationData>();
    creation_data->storage_directory = storage_directory_.GetPath();
    creation_data->client = std::move(client);
    profile->SetUserData(kCreationData, std::move(creation_data));
    DownloadServiceFactory::GetInstance()->SetTestingFactoryAndUse(
        profile, TestDownloadServiceFactory);
    download_service_ =
        DownloadServiceFactory::GetInstance()->GetForBrowserContext(profile);
  }

  // TODO(xingliu): Remove if we can.
  void TearDownOnMainThread() override {}

 protected:
  download::DownloadService* download_service() { return download_service_; }

 private:
  // The download service to verify.
  download::DownloadService* download_service_;

  // Directory for download files and database.
  base::ScopedTempDir storage_directory_;

  // Owned by |download_service_|.
  MockClient* client_;

  DISALLOW_COPY_AND_ASSIGN(DownloadServiceTest);
};

// Test to verify basic downloads.
IN_PROC_BROWSER_TEST_F(DownloadServiceTest, BasicDownloads) {
  GURL url = GURL("http://example.com/download_file");
  content::TestDownloadRequestHandler server(url);
  content::TestDownloadRequestHandler::Parameters server_params;
  server_params.size = 1024; /* 1MB file. */
  server.StartServing(server_params);

  content::DownloadManager* download_manager = GetDownloadManager(browser());

  content::DownloadTestObserverTerminal download_terminal_observer(
      download_manager, 1u, /* wait_count */
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);

  download::DownloadParams download_params;
  download_params.request_params.url = url;
  download_service()->StartDownload(download_params);
  download_terminal_observer.WaitForFinished();

  // Verify downloads.
  std::vector<content::DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  EXPECT_EQ(1u, downloads.size());
  EXPECT_EQ(content::DownloadItem::DownloadState::COMPLETE,
            downloads[0]->GetState());
}

}  // namespace
