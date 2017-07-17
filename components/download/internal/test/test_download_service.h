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

  void set_is_ready(bool is_ready) { is_ready_ = is_ready; }

  void set_failed_download(const std::string& failed_download_id) {
    failed_download_id_ = failed_download_id;
  }

 private:
  // Begin the download, raising success/failure events.
  void ProcessDownload();

  // Use StartCallback to notify the result of starting the download.
  void HandleStartDownloadResponse(const DownloadParams& params,
                                   DownloadParams::StartResult result);

  bool is_ready_;
  std::string failed_download_id_;
  std::unique_ptr<ServiceConfig> service_config_;

  std::list<DownloadParams> downloads_;

  DISALLOW_COPY_AND_ASSIGN(TestDownloadService);
};

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DOWNLOAD_SERVICE_H_
