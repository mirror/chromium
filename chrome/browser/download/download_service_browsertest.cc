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
#include "base/run_loop.h"
#include "base/supports_user_data.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/power_monitor_test_base.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/download/download_task_scheduler_impl.h"
#include "chrome/browser/download/navigation_monitor_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/download/content/test/test_utils.h"
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
#include "net/base/network_change_notifier.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

using ConnectionType = net::NetworkChangeNotifier::ConnectionType;

namespace {

const char kCreationData[] = "DownloadServiceTestCreationData";

// Generic class to run a message loop to wait for events and unblock on certain
// condition.
// Call DoneIfNeed() to validate the condition.
template <typename... Args>
class Waiter {
 public:
  typedef base::Callback<bool(Args...)> ConditionValidator;
  Waiter() = default;
  virtual ~Waiter() {}

  // Start to wait, will block the message loop.
  void Wait(ConditionValidator validator) {
    if (waiting_)
      return;
    DCHECK(validator_.is_null());
    validator_ = validator;
    waiting_ = true;
    LOG(ERROR) << "@@@ BLA!!";
    run_loop_.reset(
        new base::RunLoop(base::RunLoop::Type::kNestableTasksAllowed));
    run_loop_->Run();
  }

  // Unblock if condition met.
  void DoneIfNeeded(Args... args) {
    if (!waiting_)
      return;
    DCHECK(!validator_.is_null());

    if (validator_.Run(args...)) {
      waiting_ = false;
      LOG(ERROR) << "@@@ quit!!";
      run_loop_->Quit();
      validator_ = ConditionValidator();
    }
  }

 private:
  ConditionValidator validator_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool waiting_ = false;

  DISALLOW_COPY_AND_ASSIGN(Waiter);
};

enum class DownloadCondition {
  NONE = 0,
  STARTED,
  COMPLETED,
  FAILED,
};

// Wait for a certain number of downloads to meet specific DownloadCondition.
class DownloadCountWaiter : public Waiter<DownloadCondition, size_t> {
 public:
  DownloadCountWaiter(DownloadCondition condition, size_t wait_count)
      : Waiter<DownloadCondition, size_t>(),
        wait_condition_(condition),
        wait_count_(wait_count) {}

  ~DownloadCountWaiter() override = default;

  void Wait() {
    auto wait_callback =
        base::Bind(&DownloadCountWaiter::CheckDone, base::Unretained(this));
    Waiter::Wait(wait_callback);
  }

 private:
  bool CheckDone(DownloadCondition condition, size_t current_count) {
    LOG(ERROR) << "@@@ " << __func__;
    return condition == wait_condition_ && current_count >= wait_count_;
  }

  DownloadCondition wait_condition_;
  size_t wait_count_;

  DISALLOW_COPY_AND_ASSIGN(DownloadCountWaiter);
};

class TestClient : public download::test::EmptyClient {
 public:
  typedef std::vector<std::string> StartedDownloads;
  typedef std::map<std::string, download::CompletionInfo> CompletedDownloads;
  typedef std::map<std::string, download::Client::FailureReason>
      FailedDownloads;

  TestClient() = default;
  ~TestClient() override = default;

  // Get the number of downloads in certain condition.
  size_t CountDownloads(DownloadCondition condition) {
    switch (condition) {
      case DownloadCondition::STARTED:
        return started_downloads_.size();
      case DownloadCondition::COMPLETED:
        return completed_downloads_.size();
      case DownloadCondition::FAILED:
        return failed_downloads_.size();
      case DownloadCondition::NONE:
        break;
    }
    NOTREACHED();
    return 0u;
  }

  const StartedDownloads& started_downloads() const {
    return started_downloads_;
  }

  const CompletedDownloads& completed_downloads() const {
    return completed_downloads_;
  }

  const FailedDownloads& failed_downloads() const { return failed_downloads_; }

  // Wait for number of downloads meet the download condition.
  void Wait(DownloadCondition condition, size_t wait_count) {
    // See if we have done.
    if (CountDownloads(condition) >= wait_count) {
      return;
    }

    count_waiter_ =
        base::MakeUnique<DownloadCountWaiter>(condition, wait_count);
    count_waiter_->Wait();
  }

  // download::test::EmptyClient implementation.
  download::Client::ShouldDownload OnDownloadStarted(
      const std::string& guid,
      const std::vector<GURL>& url_chain,
      const scoped_refptr<const net::HttpResponseHeaders>& headers) override {
    started_downloads_.emplace_back(guid);

    if (count_waiter_)
      count_waiter_->DoneIfNeeded(DownloadCondition::STARTED,
                                  started_downloads_.size());

    return download::Client::ShouldDownload::CONTINUE;
  }

  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason) override {
    failed_downloads_[guid] = reason;

    if (count_waiter_)
      count_waiter_->DoneIfNeeded(DownloadCondition::FAILED,
                                  failed_downloads_.size());
  }

  void OnDownloadSucceeded(
      const std::string& guid,
      const download::CompletionInfo& completion_info) override {
    completed_downloads_.emplace(guid, completion_info);
    if (count_waiter_)
      count_waiter_->DoneIfNeeded(DownloadCondition::COMPLETED,
                                  completed_downloads_.size());
  }

 private:
  StartedDownloads started_downloads_;
  CompletedDownloads completed_downloads_;
  FailedDownloads failed_downloads_;

  std::unique_ptr<DownloadCountWaiter> count_waiter_;

  DISALLOW_COPY_AND_ASSIGN(TestClient);
};

// Used to pass data to download service factory in test. The test factory
// functor can not capture test objects.
struct CreationData : public base::SupportsUserData::Data {
  CreationData() = default;
  ~CreationData() override = default;

  base::FilePath downloads_directory;
  base::FilePath db_directory;
  std::unique_ptr<download::Client> client;

 private:
  DISALLOW_COPY_AND_ASSIGN(CreationData);
};

std::unique_ptr<KeyedService> BuildDownloadServiceForTest(
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
  download::NavigationMonitor* navigation_monitor =
      NavigationMonitorFactory::GetForBrowserContext(context);

  return base::WrapUnique<download::DownloadService>(
      download::test::CreateDownloadServiceForTest(
          std::move(client_map), context, downloads_directory, db_directory,
          background_task_runner, std::move(task_scheduler),
          navigation_monitor));
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
    content::DownloadManager* manager =
        content::BrowserContext::GetDownloadManager(browser()->profile());
    DownloadPrefs::FromDownloadManager(manager)->ResetAutoOpen();

    // Network hook.
    ChangeNetworkType(ConnectionType::CONNECTION_WIFI);

    // Setup the service and the client.
    auto client = base::MakeUnique<TestClient>();
    client_ = client.get();
    auto creation_data = base::MakeUnique<CreationData>();
    creation_data->downloads_directory = downloads_directory_.GetPath();
    creation_data->db_directory = db_directory_.GetPath();
    creation_data->client = std::move(client);
    profile->SetUserData(kCreationData, std::move(creation_data));
    DownloadServiceFactory::GetInstance()->SetTestingFactoryAndUse(
        profile, BuildDownloadServiceForTest);
    download_service_ =
        DownloadServiceFactory::GetInstance()->GetForBrowserContext(profile);
  }

  void TearDownOnMainThread() override { download_service_->Shutdown(); }

 protected:
  download::DownloadService* download_service() { return download_service_; }
  TestClient* client() { return client_; }

  void ChangeNetworkType(ConnectionType type) {
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(type);
  }

  void SimulateBatteryChange(bool on_battery_power) {
    // TODO(xingliu): Figure out appropriate ways to simulate battery change.
    NOTIMPLEMENTED();
  }

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
  client()->Wait(DownloadCondition::COMPLETED, 1u);
  const TestClient::CompletedDownloads& downloads =
      client()->completed_downloads();
  EXPECT_EQ(1u, downloads.size());
  EXPECT_TRUE(downloads.find(download_params.guid) != downloads.end());
}

}  // namespace
