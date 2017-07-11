// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/test/test_download_service.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"

namespace download {
namespace test {

TestDownloadService::TestDownloadService()
    : is_ready_(false), observer_(nullptr), file_size_(123456789u) {}

TestDownloadService::~TestDownloadService() = default;

const ServiceConfig& TestDownloadService::GetConfig() {
  return service_config_;
}

void TestDownloadService::OnStartScheduledTask(
    DownloadTaskType task_type,
    const TaskFinishedCallback& callback) {}

bool TestDownloadService::OnStopScheduledTask(DownloadTaskType task_type) {
  return true;
}

DownloadService::ServiceStatus TestDownloadService::GetStatus() {
  return is_ready_ ? DownloadService::ServiceStatus::READY
                   : DownloadService::ServiceStatus::STARTING_UP;
}

void TestDownloadService::StartDownload(const DownloadParams& download_params) {
  if (!is_ready_) {
    OnDownloadFailed(download_params.guid);
    return;
  }
  downloads_.push_back(download_params);
  ProcessDownload();
}

void TestDownloadService::PauseDownload(const std::string& guid) {}

void TestDownloadService::ResumeDownload(const std::string& guid) {}

void TestDownloadService::CancelDownload(const std::string& guid) {
  for (auto iter = downloads_.begin(); iter != downloads_.end(); ++iter) {
    if (iter->guid == guid) {
      downloads_.erase(iter);
      return;
    }
  }
}

void TestDownloadService::ChangeDownloadCriteria(
    const std::string& guid,
    const SchedulingParams& params) {}

}  // namespace test
}  // namespace download
