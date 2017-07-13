// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_

#include <list>

#include "components/download/public/client.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"

namespace download {
namespace test {

// Implementation of DownloadService used for testing.
class TestDownloadService : public DownloadService {
 public:
  TestDownloadService(Client* client);
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

  // Setters for variables.
  void set_is_ready(bool is_ready) { is_ready_ = is_ready; }

  void set_failed_download(const std::string failed_download_id) {
    std::string failed_download_id_(failed_download_id);
  }

  void set_file_size(uint64_t file_size) { file_size_ = file_size; }

 private:
  // Begin the download, raising success/failure events.
  void ProcessDownload();

  // Notify the observer a download has succeeded.
  void OnDownloadSucceeded(const std::string& guid,
                           const base::FilePath& file_path,
                           uint64_t file_size);

  // Notify the observer a download has failed.
  void OnDownloadFailed(const std::string& guid,
                        Client::FailureReason failure_reason);

  bool is_ready_;
  const std::string failed_download_id_;
  uint64_t file_size_;
  std::unique_ptr<ServiceConfig> service_config_;

  Client* client_;

  std::list<DownloadParams> downloads_;

  DISALLOW_COPY_AND_ASSIGN(TestDownloadService);
};

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_
