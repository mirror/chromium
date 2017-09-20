// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/supports_user_data.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/download/download_task_scheduler_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/download/content/factory/test_download_service_factory.h"
#include "components/download/public/download_metadata.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "components/download/public/test/empty_client.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/test/test_download_request_handler.h"
#include "content/public/test/test_utils.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {

const char kCreationData[] = "DownloadServiceTestCreationData";

// Provides public access to data exposed by download service.
class TestClient : public download::test::EmptyClient {
 public:
  typedef std::map<std::string, download::CompletionInfo> CompletedDownloads;
  typedef std::map<std::string, download::Client::FailureReason>
      FailedDownloads;

  TestClient() = default;
  ~TestClient() override = default;

  // download::test::EmptyClient overrides.
  download::Client::ShouldDownload OnDownloadStarted(
      const std::string& guid,
      const std::vector<GURL>& url_chain,
      const scoped_refptr<const net::HttpResponseHeaders>& headers) override {
    return download::Client::ShouldDownload::CONTINUE;
  }

  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason) override {
    failed_downloads_[guid] = reason;
  }

  void OnDownloadSucceeded(
      const std::string& guid,
      const download::CompletionInfo& completion_info) override {
    completed_downloads_.emplace(guid, completion_info);
  }

  const std::vector<std::string>& started_downloads_guid() const {
    return started_downloads_guid_;
  }

  const CompletedDownloads& completed_downloads() const {
    return completed_downloads_;
  }

  const FailedDownloads& failed_downloads() const { return failed_downloads_; }

 private:
  std::vector<std::string> started_downloads_guid_;
  std::map<std::string, download::CompletionInfo> completed_downloads_;
  std::map<std::string, download::Client::FailureReason> failed_downloads_;

  DISALLOW_COPY_AND_ASSIGN(TestClient);
};

content::DownloadManager* GetDownloadManager(Browser* browser) {
  DCHECK(browser);
  return content::BrowserContext::GetDownloadManager(browser->profile());
}

void RunMessageLoop() {
  base::RunLoop run_loop;
  content::RunThisRunLoop(&run_loop);
}

// Used to pass data to download service factory in test.
struct CreationData : public base::SupportsUserData::Data {
  base::FilePath downloads_directory;
  base::FilePath db_directory;
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

  auto downloads_directory = creation_data->downloads_directory;
  auto db_directory = creation_data->db_directory;

  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});

  // TODO(xingliu): Figure out if we need to use a fake task scheduler.
  std::unique_ptr<download::TaskScheduler> task_scheduler;
  task_scheduler = base::MakeUnique<DownloadTaskSchedulerImpl>(context);

  return base::WrapUnique<download::DownloadService>(
      download::test::CreateDownloadServiceForTest(
          std::move(client_map), context, downloads_directory, db_directory,
          background_task_runner, std::move(task_scheduler)));
}

// Test fixture to verify components/download API.
class DownloadServiceTest : public InProcessBrowserTest {
 public:
  DownloadServiceTest() {}

  void SetUpOnMainThread() override {
    // Setup the directory and download manager.
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());
      ASSERT_TRUE(db_directory_.CreateUniqueTempDir());
    }
    Profile* profile = browser()->profile();
    profile->GetPrefs()->SetBoolean(prefs::kPromptForDownload, false);
    content::DownloadManager* manager = GetDownloadManager(browser());
    DownloadPrefs::FromDownloadManager(manager)->ResetAutoOpen();

    // Setup the service and the client.
    auto client = base::MakeUnique<TestClient>();
    client_ = client.get();
    auto creation_data = base::MakeUnique<CreationData>();
    creation_data->downloads_directory = downloads_directory_.GetPath();
    creation_data->db_directory = db_directory_.GetPath();
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

  // Directories for download files and database.
  base::ScopedTempDir downloads_directory_;
  base::ScopedTempDir db_directory_;

  // Owned by |download_service_|.
  TestClient* client_;

  DISALLOW_COPY_AND_ASSIGN(DownloadServiceTest);
};

// Test to verify basic download through download service API.
IN_PROC_BROWSER_TEST_F(DownloadServiceTest, BasicDownloads) {
  GURL url = GURL("http://example.com/download_file");
  content::TestDownloadRequestHandler server(url);
  content::TestDownloadRequestHandler::Parameters server_params;
  server_params.size = 1024; /* 1MB file. */
  server.StartServing(server_params);

  download::DownloadParams download_params;
  download_params.request_params.url = url;
  download_params.guid = base::GenerateGUID();
  download_params.client = download::DownloadClient::TEST;
  download_params.traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);

  download_service()->StartDownload(download_params);

  RunMessageLoop();

  // Verify downloads.
  // TODO(xingliu): We removed all download in content/, just to verify
  // the files in the downloads directory. and callbacks.
  content::DownloadManager* download_manager = GetDownloadManager(browser());
  std::vector<content::DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  EXPECT_EQ(1u, downloads.size());
  EXPECT_EQ(content::DownloadItem::DownloadState::COMPLETE,
            downloads[0]->GetState());
}

}  // namespace
