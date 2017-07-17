// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/test/test_download_service.h"
#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/public/client.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "components/download/public/service_config.h"

namespace download {
namespace test {

namespace {

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

}  // namespace

TestDownloadService::TestDownloadService()
    : is_ready_(false), file_size_(123456789u), service_config_(nullptr) {
  std::unique_ptr<ServiceConfig> service_config_(new TestServiceConfig());
}

TestDownloadService::~TestDownloadService() = default;

const ServiceConfig& TestDownloadService::GetConfig() {
  return *service_config_;
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
    HandleStartDownloadResponse(download_params,
                                DownloadParams::StartResult::INTERNAL_ERROR);
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
      HandleStartDownloadResponse(
          *iter, DownloadParams::StartResult::CLIENT_CANCELLED);
      return;
    }
  }
}

void TestDownloadService::ChangeDownloadCriteria(
    const std::string& guid,
    const SchedulingParams& params) {}

void TestDownloadService::ProcessDownload() {
  if (!is_ready_ || downloads_.empty())
    return;
  DownloadParams params = downloads_.front();
  downloads_.pop_front();
  if (!failed_download_id_.empty() && params.guid == failed_download_id_)
    HandleStartDownloadResponse(params,
                                DownloadParams::StartResult::UNEXPECTED_GUID);
  else
    HandleStartDownloadResponse(params, DownloadParams::StartResult::ACCEPTED);
}

void TestDownloadService::HandleStartDownloadResponse(
    const DownloadParams& params,
    DownloadParams::StartResult result) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(params.callback, params.guid, result));
}

}  // namespace test
}  // namespace download
