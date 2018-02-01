// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_update_engine_client.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

// Delay between successive state transitions during AU.
constexpr base::TimeDelta kStateTransitionDefaultDelay =
    base::TimeDelta::FromMilliseconds(3000);

// Delay between successive notifications about downloading progress
// during fake AU.
constexpr base::TimeDelta kStateTransitionDownloadingDelay =
    base::TimeDelta::FromMilliseconds(250);

// Size of parts of a "new" image which are downloaded each
// |kStateTransitionDownloadingDelayMs| during fake AU.
constexpr int64_t kDownloadSizeDelta = 1 << 19;

constexpr char kReleaseChannelBeta[] = "beta-channel";

FakeUpdateEngineClient::FakeUpdateEngineClient(uint32_t options)
    : update_check_result_(UpdateEngineClient::UPDATE_RESULT_SUCCESS),
      can_rollback_stub_result_(false),
      reboot_after_update_call_count_(0),
      request_update_check_call_count_(0),
      rollback_call_count_(0),
      can_rollback_call_count_(0),
      current_channel_(kReleaseChannelBeta),
      target_channel_(kReleaseChannelBeta),
      options_(options),
      weak_factory_(this) {}

FakeUpdateEngineClient::FakeUpdateEngineClient()
    : FakeUpdateEngineClient(USE_FAKE_UPDATE_RESULT | USE_FAKE_UPDATE_STATUS) {}

FakeUpdateEngineClient::~FakeUpdateEngineClient() = default;

void FakeUpdateEngineClient::Init(dbus::Bus* bus) {}

void FakeUpdateEngineClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeUpdateEngineClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool FakeUpdateEngineClient::HasObserver(const Observer* observer) const {
  return observers_.HasObserver(observer);
}

void FakeUpdateEngineClient::RequestUpdateCheck(
    const UpdateCheckCallback& callback) {
  request_update_check_call_count_++;
  if (options_ & USE_FAKE_UPDATE_RESULT) {
    callback.Run(update_check_result_);
    return;
  }
  if (last_status_.status != UPDATE_STATUS_IDLE) {
    callback.Run(UPDATE_RESULT_FAILED);
    return;
  }
  last_status_.status = UPDATE_STATUS_CHECKING_FOR_UPDATE;
  last_status_.download_progress = 0.0;
  last_status_.last_checked_time = 0;
  last_status_.new_size = 0;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&FakeUpdateEngineClient::StateTransition,
                     weak_factory_.GetWeakPtr()),
      kStateTransitionDefaultDelay);
  callback.Run(UPDATE_RESULT_SUCCESS);
}

void FakeUpdateEngineClient::FakeUpdateEngineClient::RebootAfterUpdate() {
  reboot_after_update_call_count_++;
}

void FakeUpdateEngineClient::Rollback() {
  rollback_call_count_++;
}

void FakeUpdateEngineClient::CanRollbackCheck(
    const RollbackCheckCallback& callback) {
  can_rollback_call_count_++;
  callback.Run(can_rollback_stub_result_);
}

UpdateEngineClient::Status FakeUpdateEngineClient::GetLastStatus() {
  if (options_ & USE_FAKE_UPDATE_STATUS) {
    if (status_queue_.empty())
      return default_status_;

    UpdateEngineClient::Status last_status = status_queue_.front();
    status_queue_.pop();
    return last_status;
  } else {
    return last_status_;
  }
}

void FakeUpdateEngineClient::NotifyObserversThatStatusChanged(
    const UpdateEngineClient::Status& status) {
  for (auto& observer : observers_)
    observer.UpdateStatusChanged(status);
}

void FakeUpdateEngineClient::
    NotifyUpdateOverCellularOneTimePermissionGranted() {
  for (auto& observer : observers_)
    observer.OnUpdateOverCellularOneTimePermissionGranted();
}

void FakeUpdateEngineClient::SetChannel(const std::string& target_channel,
                                        bool is_powerwash_allowed) {
  VLOG(1) << "Requesting to set channel: "
          << "target_channel=" << target_channel << ", "
          << "is_powerwash_allowed=" << is_powerwash_allowed;
  target_channel_ = target_channel;
}

void FakeUpdateEngineClient::GetChannel(bool get_current_channel,
                                        const GetChannelCallback& callback) {
  VLOG(1) << "Requesting to get channel, get_current_channel="
          << get_current_channel;
  callback.Run(get_current_channel ? current_channel_ : target_channel_);
}

void FakeUpdateEngineClient::GetEolStatus(
    const GetEolStatusCallback& callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(callback, update_engine::EndOfLifeStatus::kSupported));
}

void FakeUpdateEngineClient::SetUpdateOverCellularPermission(
    bool allowed,
    const base::Closure& callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, callback);
}

void FakeUpdateEngineClient::SetUpdateOverCellularOneTimePermission(
    const std::string& target_version,
    int64_t target_size,
    const UpdateOverCellularOneTimePermissionCallback& callback) {
  callback.Run(true);
}

void FakeUpdateEngineClient::set_default_status(
    const UpdateEngineClient::Status& status) {
  DCHECK(options_ & USE_FAKE_UPDATE_STATUS);
  default_status_ = status;
}

void FakeUpdateEngineClient::set_update_check_result(
    const UpdateEngineClient::UpdateCheckResult& result) {
  DCHECK(options_ & USE_FAKE_UPDATE_RESULT);
  update_check_result_ = result;
}

void FakeUpdateEngineClient::StateTransition() {
  UpdateStatusOperation next_status = UPDATE_STATUS_ERROR;
  base::TimeDelta delay = kStateTransitionDefaultDelay;
  switch (last_status_.status) {
    case UPDATE_STATUS_ERROR:
    case UPDATE_STATUS_IDLE:
    case UPDATE_STATUS_UPDATED_NEED_REBOOT:
    case UPDATE_STATUS_REPORTING_ERROR_EVENT:
    case UPDATE_STATUS_ATTEMPTING_ROLLBACK:
    case UPDATE_STATUS_NEED_PERMISSION_TO_UPDATE:
      return;
    case UPDATE_STATUS_CHECKING_FOR_UPDATE:
      next_status = UPDATE_STATUS_UPDATE_AVAILABLE;
      break;
    case UPDATE_STATUS_UPDATE_AVAILABLE:
      next_status = UPDATE_STATUS_DOWNLOADING;
      break;
    case UPDATE_STATUS_DOWNLOADING:
      if (last_status_.download_progress >= 1.0) {
        next_status = UPDATE_STATUS_VERIFYING;
      } else {
        next_status = UPDATE_STATUS_DOWNLOADING;
        last_status_.download_progress += 0.01;
        last_status_.new_size = kDownloadSizeDelta;
        delay = kStateTransitionDownloadingDelay;
      }
      break;
    case UPDATE_STATUS_VERIFYING:
      next_status = UPDATE_STATUS_FINALIZING;
      break;
    case UPDATE_STATUS_FINALIZING:
      next_status = UPDATE_STATUS_IDLE;
      break;
  }
  last_status_.status = next_status;
  for (auto& observer : observers_)
    observer.UpdateStatusChanged(last_status_);
  if (last_status_.status != UPDATE_STATUS_IDLE) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&FakeUpdateEngineClient::StateTransition,
                       weak_factory_.GetWeakPtr()),
        delay);
  }
}

}  // namespace chromeos
