// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_service_factory.h"

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "chrome/browser/download/download_task_scheduler_impl.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "components/download/content/factory/download_service_factory.h"
#include "components/download/public/clients.h"
#include "components/download/public/download_service.h"
#include "components/download/public/task_scheduler.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/download/service/download_task_scheduler.h"
#endif

#include "base/guid.h"
#include "base/strings/string_util.h"
#include "components/download/public/client.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"

class TestClient : public download::Client {
 public:
  TestClient(content::BrowserContext* context) : browser_context_(context) {}
  ~TestClient() override = default;

  void StartDownload() {
    // Starts a download for manual test.
    download::DownloadService* download_service =
        DownloadServiceFactory::GetForBrowserContext(browser_context_);
    download::DownloadParams params;
    params.guid = base::ToUpperASCII(base::GenerateGUID());
    params.client = download::DownloadClient::TEST;
    LOG(ERROR) << "@@@ creating GUID = " << params.guid;
    params.request_params.url = GURL(
    //     "https://notepad-plus-plus.org/repository/7.x/7.3.1/npp.7.3.1.bin.7z");
    "http://eclipse.mirror.rafal.ca/technology/epp/downloads/release/neon/3/eclipse-java-neon-3-win32.zip");

    // params.scheduling_params.battery_requirements = download::SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE;
    params.scheduling_params.network_requirements = download::SchedulingParams::NetworkRequirements::UNMETERED;

    download_service->StartDownload(params);
    // download_service->PauseDownload("5C9EB3A6-88FD-482E-AC89-E0FE86FEFA90");
  }

  // Client implementation.
  void OnServiceInitialized(
      const std::vector<std::string>& outstanding_download_guids) override {
    LOG(ERROR) << "@@@ OnServiceInitialized";
    DCHECK(browser_context_);
    StartDownload();

    /*StartDownload();
    StartDownload();
    StartDownload();
    StartDownload();
    */
  }

  download::Client::ShouldDownload OnDownloadStarted(
      const std::string& guid,
      const std::vector<GURL>& url_chain,
      const scoped_refptr<const net::HttpResponseHeaders>& headers) override {
    LOG(ERROR) << "@@@ OnDownloadStarted, guid = " << guid;

    return download::Client::ShouldDownload::CONTINUE;
  }
  void OnDownloadUpdated(const std::string& guid,
                         uint64_t bytes_downloaded) override {
    LOG(ERROR) << "@@@ OnDownloadUpdated, guid = " << guid
               << " , bytes_downloaded = " << bytes_downloaded;
  }
  void OnDownloadFailed(const std::string& guid,
                        download::Client::FailureReason reason) override {
    LOG(ERROR) << "@@@ OnDownloadFailed, guid = " << guid
               << " , reason = " << static_cast<int>(reason);
  }
  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& path,
                           uint64_t size) override {
    LOG(ERROR) << "@@@ OnDownloadSucceeded, , guid = " << guid
               << ", path = " << path.value() << ", size = " << size;
  }

 private:
  content::BrowserContext* browser_context_;
  DISALLOW_COPY_AND_ASSIGN(TestClient);
};

// static
DownloadServiceFactory* DownloadServiceFactory::GetInstance() {
  LOG(ERROR) << "@@@ DownloadServiceFactory::GetInstance()!";
  return base::Singleton<DownloadServiceFactory>::get();
}

// static
download::DownloadService* DownloadServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<download::DownloadService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

DownloadServiceFactory::DownloadServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "download::DownloadService",
          BrowserContextDependencyManager::GetInstance()) {}

DownloadServiceFactory::~DownloadServiceFactory() = default;

KeyedService* DownloadServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  LOG(ERROR) << "@@@ DownloadServiceFactory::BuildServiceInstanceFor!";

  auto clients = base::MakeUnique<download::DownloadClientMap>();
  // TODO(dtrainor): Register all clients here.

  // Register a test client.
  clients->emplace(download::DownloadClient::TEST,
                   base::MakeUnique<TestClient>(context));

  auto* download_manager = content::BrowserContext::GetDownloadManager(context);

  base::FilePath storage_dir;
  if (!context->IsOffTheRecord() && !context->GetPath().empty()) {
    storage_dir =
        context->GetPath().Append(chrome::kDownloadServiceStorageDirname);
    LOG(ERROR) << "@@@ storage_dir = " << storage_dir.value();
  }

  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});

  std::unique_ptr<download::TaskScheduler> task_scheduler;

#if defined(OS_ANDROID)
  task_scheduler = base::MakeUnique<download::android::DownloadTaskScheduler>();
#else
  task_scheduler = base::MakeUnique<DownloadTaskSchedulerImpl>(context);
#endif

  return download::CreateDownloadService(std::move(clients), download_manager,
                                         storage_dir, background_task_runner,
                                         std::move(task_scheduler));
}

content::BrowserContext* DownloadServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
