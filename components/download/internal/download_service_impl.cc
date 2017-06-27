// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/download_service_impl.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "components/download/internal/controller_impl.h"
#include "components/download/internal/startup_status.h"
#include "components/download/internal/stats.h"

namespace download {

DownloadServiceImpl::DownloadServiceImpl(std::unique_ptr<Configuration> config,
                                         std::unique_ptr<Controller> controller)
    : config_(std::move(config)),
      controller_(std::move(controller)),
      service_config_(config_.get()),
      startup_completed_(false) {
  controller_->SetStartupListener(this);
  controller_->Initialize();
}

DownloadServiceImpl::~DownloadServiceImpl() = default;

const ServiceConfig& DownloadServiceImpl::GetConfig() {
  return service_config_;
}

void DownloadServiceImpl::OnStartScheduledTask(
    DownloadTaskType task_type,
    const TaskFinishedCallback& callback) {
  auto pending_action =
      base::Bind(&Controller::OnStartScheduledTask,
                 base::Unretained(controller_.get()), task_type, callback);
  if (startup_completed_) {
    pending_action.Run();
    return;
  }

  bool found_task_entry = false;
  for (size_t i = 0; i < pending_actions_.size(); i++) {
    if (pending_actions_[i].second == task_type) {
      pending_actions_[i].first = pending_action;
      found_task_entry = true;
      break;
    }
  }

  if (!found_task_entry) {
    pending_actions_.push_back(std::make_pair(pending_action, task_type));
  }
}

bool DownloadServiceImpl::OnStopScheduledTask(DownloadTaskType task_type) {
  if (startup_completed_) {
    return controller_->OnStopScheduledTask(task_type);
  }

  int task_entry_index = -1;
  for (size_t i = 0; i < pending_actions_.size(); i++) {
    if (pending_actions_[i].second == task_type) {
      task_entry_index = i;
      break;
    }
  }

  if (task_entry_index != -1) {
    pending_actions_[task_entry_index].first.Run();
    pending_actions_.erase(pending_actions_.begin() + task_entry_index);
  }

  return true;
}

DownloadService::ServiceStatus DownloadServiceImpl::GetStatus() {
  if (!controller_->GetStartupStatus()->Complete())
    return DownloadService::ServiceStatus::STARTING_UP;

  if (!controller_->GetStartupStatus()->Ok())
    return DownloadService::ServiceStatus::UNAVAILABLE;

  return DownloadService::ServiceStatus::READY;
}

void DownloadServiceImpl::StartDownload(const DownloadParams& download_params) {
  stats::LogServiceApiAction(download_params.client,
                             stats::ServiceApiAction::START_DOWNLOAD);
  DCHECK_EQ(download_params.guid, base::ToUpperASCII(download_params.guid));
  auto pending_action =
      base::Bind(&Controller::StartDownload,
                 base::Unretained(controller_.get()), download_params);
  if (startup_completed_) {
    pending_action.Run();
  } else {
    pending_actions_.push_back(
        std::make_pair(pending_action, DownloadTaskType::NONE));
  }
}

void DownloadServiceImpl::PauseDownload(const std::string& guid) {
  stats::LogServiceApiAction(controller_->GetOwnerOfDownload(guid),
                             stats::ServiceApiAction::PAUSE_DOWNLOAD);
  DCHECK_EQ(guid, base::ToUpperASCII(guid));
  auto pending_action = base::Bind(&Controller::PauseDownload,
                                   base::Unretained(controller_.get()), guid);
  if (startup_completed_) {
    pending_action.Run();
  } else {
    pending_actions_.push_back(
        std::make_pair(pending_action, DownloadTaskType::NONE));
  }
}

void DownloadServiceImpl::ResumeDownload(const std::string& guid) {
  stats::LogServiceApiAction(controller_->GetOwnerOfDownload(guid),
                             stats::ServiceApiAction::RESUME_DOWNLOAD);
  DCHECK_EQ(guid, base::ToUpperASCII(guid));
  auto pending_action = base::Bind(&Controller::ResumeDownload,
                                   base::Unretained(controller_.get()), guid);
  if (startup_completed_) {
    pending_action.Run();
  } else {
    pending_actions_.push_back(
        std::make_pair(pending_action, DownloadTaskType::NONE));
  }
}

void DownloadServiceImpl::CancelDownload(const std::string& guid) {
  stats::LogServiceApiAction(controller_->GetOwnerOfDownload(guid),
                             stats::ServiceApiAction::CANCEL_DOWNLOAD);
  DCHECK_EQ(guid, base::ToUpperASCII(guid));
  auto pending_action = base::Bind(&Controller::CancelDownload,
                                   base::Unretained(controller_.get()), guid);
  if (startup_completed_) {
    pending_action.Run();
  } else {
    pending_actions_.push_back(
        std::make_pair(pending_action, DownloadTaskType::NONE));
  }
}

void DownloadServiceImpl::ChangeDownloadCriteria(
    const std::string& guid,
    const SchedulingParams& params) {
  stats::LogServiceApiAction(controller_->GetOwnerOfDownload(guid),
                             stats::ServiceApiAction::CHANGE_CRITERIA);
  DCHECK_EQ(guid, base::ToUpperASCII(guid));
  auto pending_action =
      base::Bind(&Controller::ChangeDownloadCriteria,
                 base::Unretained(controller_.get()), guid, params);
  if (startup_completed_) {
    pending_action.Run();
  } else {
    pending_actions_.push_back(
        std::make_pair(pending_action, DownloadTaskType::NONE));
  }
}

void DownloadServiceImpl::OnControllerInitialized(
    const StartupStatus& startup_status) {
  DCHECK(startup_status.Complete());

  while (!pending_actions_.empty()) {
    auto& action = pending_actions_.front().first;
    auto& task_type = pending_actions_.front().second;
    if (startup_status.Ok()) {
      action.Run();
    } else {
      // Drop all the pending calls except the scheduled tasks.
      if (task_type != DownloadTaskType::NONE) {
        action.Run();
      }
    }

    pending_actions_.pop_front();
  }
  startup_completed_ = true;
}

}  // namespace download
