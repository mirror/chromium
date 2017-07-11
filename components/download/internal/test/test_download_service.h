// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_

#include <list>

#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "components/download/public/service_config.h"

namespace download {
namespace test {

// Implementation of DownloadService used for testing.
class TestDownloadService : public DownloadService {
 public:
  // Implementation of ServiceConfig used for testing.
  class TestServiceConfig : public ServiceConfig {
   public:
    TestServiceConfig() = default;
    ~TestServiceConfig() override = default;

    uint32_t GetMaxScheduledDownloadsPerClient() const override { return 0; }
    const base::TimeDelta& GetFileKeepAliveTime() const override {
      return time_delta_;
    }

   private:
    base::TimeDelta time_delta_;
  };

  // Interface for an observer listening to download success/failure events.
  class Observer {
   public:
    // Called when the download has succeeded.
    virtual void OnDownloadSucceeded(const std::string& download_id,
                                     const base::FilePath& file_path,
                                     uint64_t file_size) = 0;

    // Called when the download has failed.
    virtual void OnDownloadFailed(const std::string& download_id) = 0;
  };

  TestDownloadService();
  ~TestDownloadService() override;

  // DownloadService implementation.
  const ServiceConfig& GetConfig() override;
  void OnStartScheduledTask(DownloadTaskType task_type,
                            const TaskFinishedCallback& callback) override;
  bool OnStopScheduledTask(DownloadTaskType task_type) override;
  DownloadService::ServiceStatus GetStatus() override;
  void StartDownload(const DownloadParams& download_params) override;
  void PauseDownload(const std::string& guid) override;
  void ResumeDownload(const std::string& guid) override;
  void CancelDownload(const std::string& guid) override;
  void ChangeDownloadCriteria(const std::string& guid,
                              const SchedulingParams& params) override;

 protected:
  // Whether the service is ready to download.
  bool is_ready_;

  // The observer that gets notified of the download success/failure events.
  Observer* observer_;

  // The id of the download that will fail.
  const std::string& failed_download_id_;

  // The size of the files.
  uint64_t file_size_;

 private:
  // Begin the download, raising success/failure events.
  void ProcessDownload() {
    if ((!is_ready_) || downloads_.empty())
      return;
    DownloadParams params = downloads_.front();
    downloads_.pop_front();
    if ((!failed_download_id_.empty()) && (params.guid == failed_download_id_))
      OnDownloadFailed(params.guid);
    else
      OnDownloadSucceeded(params.guid, base::FilePath(), file_size_);
  }

  // Notify the observer a download has succeeded.
  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& file_path,
                           uint64_t file_size) {
    if (observer_)
      observer_->OnDownloadSucceeded(guid, file_path, file_size);
  }

  // Notify the observer a download has failed.
  void OnDownloadFailed(const std::string& guid) {
    if (observer_)
      observer_->OnDownloadFailed(guid);
  }

  // Initial service configuration.
  TestServiceConfig service_config_;

  // List of active downloads.
  std::list<DownloadParams> downloads_;

  DISALLOW_COPY_AND_ASSIGN(TestDownloadService);
};

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_
