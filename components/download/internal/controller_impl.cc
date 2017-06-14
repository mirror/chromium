// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/controller_impl.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/internal/client_set.h"
#include "components/download/internal/config.h"
#include "components/download/internal/entry.h"
#include "components/download/internal/entry_utils.h"
#include "components/download/internal/model.h"
#include "components/download/internal/scheduler/scheduler_impl.h"
#include "components/download/internal/stats.h"
#include "components/download/public/client.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace download {
namespace {

// Helper function to transit the state of |entry| to |new_state|.
void TransitTo(Entry* entry, Entry::State new_state, Model* model) {
  DCHECK(entry);
  if (entry->state == new_state)
    return;
  entry->state = new_state;
  model->Update(*entry);
}

}  // namespace

ControllerImpl::ControllerImpl(
    std::unique_ptr<ClientSet> clients,
    std::unique_ptr<Configuration> config,
    std::unique_ptr<DownloadDriver> driver,
    std::unique_ptr<Model> model,
    std::unique_ptr<DeviceStatusListener> device_status_listener,
    std::unique_ptr<Scheduler> scheduler)
    : clients_(std::move(clients)),
      config_(std::move(config)),
      driver_(std::move(driver)),
      model_(std::move(model)),
      device_status_listener_(std::move(device_status_listener)),
      scheduler_(std::move(scheduler)) {}

ControllerImpl::~ControllerImpl() = default;

void ControllerImpl::Initialize() {
  DCHECK(!startup_status_.Complete());

  driver_->Initialize(this);
  model_->Initialize(this);
}

const StartupStatus* ControllerImpl::GetStartupStatus() {
  return &startup_status_;
}

void ControllerImpl::StartDownload(const DownloadParams& params) {
  DCHECK(startup_status_.Ok());

  if (start_callbacks_.find(params.guid) != start_callbacks_.end() ||
      model_->Get(params.guid) != nullptr) {
    HandleStartDownloadResponse(params.client, params.guid,
                                DownloadParams::StartResult::UNEXPECTED_GUID,
                                params.callback);
    return;
  }

  auto* client = clients_->GetClient(params.client);
  if (!client) {
    HandleStartDownloadResponse(params.client, params.guid,
                                DownloadParams::StartResult::UNEXPECTED_CLIENT,
                                params.callback);
    return;
  }

  uint32_t client_count =
      util::GetNumberOfEntriesForClient(params.client, model_->PeekEntries());
  if (client_count >= config_->max_scheduled_downloads) {
    HandleStartDownloadResponse(params.client, params.guid,
                                DownloadParams::StartResult::BACKOFF,
                                params.callback);
    return;
  }

  start_callbacks_[params.guid] = params.callback;
  model_->Add(Entry(params));
}

void ControllerImpl::PauseDownload(const std::string& guid) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);

  if (!entry || entry->state == Entry::State::PAUSED ||
      entry->state == Entry::State::COMPLETE ||
      entry->state == Entry::State::WATCHDOG) {
    return;
  }

  TransitTo(entry, Entry::State::PAUSED, model_.get());
  UpdateDriverState(*entry);

  // Pausing a download may yield a concurrent slot to start a new download, and
  // may change the scheduling criteria.
  ActivateMoreDownloads();
}

void ControllerImpl::ResumeDownload(const std::string& guid) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  DCHECK(entry);

  if (entry->state != Entry::State::PAUSED)
    return;

  TransitTo(entry, Entry::State::ACTIVE, model_.get());
  UpdateDriverState(*entry);

  ActivateMoreDownloads();
}

void ControllerImpl::CancelDownload(const std::string& guid) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  if (!entry)
    return;

  if (entry->state == Entry::State::NEW) {
    // Check if we're currently trying to add the download.
    DCHECK(start_callbacks_.find(entry->guid) != start_callbacks_.end());
    HandleStartDownloadResponse(entry->client, entry->guid,
                                DownloadParams::StartResult::CLIENT_CANCELLED);
    return;
  }

  HandleCompleteDownload(CompletionType::CANCEL, guid);
}

void ControllerImpl::ChangeDownloadCriteria(const std::string& guid,
                                            const SchedulingParams& params) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  if (!entry || entry->scheduling_params == params) {
    DVLOG(1) << "Try to update the same scheduling parameters.";
    return;
  }

  UpdateDriverState(*entry);

  // Update the scheduling parameters.
  entry->scheduling_params = params;
  model_->Update(*entry);

  ActivateMoreDownloads();
}

DownloadClient ControllerImpl::GetOwnerOfDownload(const std::string& guid) {
  auto* entry = model_->Get(guid);
  return entry ? entry->client : DownloadClient::INVALID;
}

void ControllerImpl::OnDriverReady(bool success) {
  DCHECK(!startup_status_.driver_ok.has_value());
  startup_status_.driver_ok = success;
  AttemptToFinalizeSetup();
}

void ControllerImpl::OnDownloadCreated(const DriverEntry& download) {
  // Only notifies the client about new downloads. Exsting download data will be
  // loaded before the driver is ready.
  if (!driver_->IsReady())
    return;

  Entry* entry = model_->Get(download.guid);
  if (!entry)
    return;

  download::Client* client = clients_->GetClient(entry->client);
  if (!client)
    return;

  using ShouldDownload = download::Client::ShouldDownload;
  ShouldDownload should_download = client->OnDownloadStarted(
      download.guid, download.url_chain, download.response_headers);
  if (should_download == ShouldDownload::ABORT) {
    HandleCompleteDownload(CompletionType::ABORT, entry->guid);
  }
}

void ControllerImpl::OnDownloadFailed(const DriverEntry& download, int reason) {
  Entry* entry = model_->Get(download.guid);
  if (!entry)
    return;

  // TODO(xingliu): More detailed failure reason on time out.
  HandleCompleteDownload(CompletionType::FAIL, download.guid);

  download::Client* client = clients_->GetClient(entry->client);
  if (client)
    client->OnDownloadFailed(download.guid);
}

void ControllerImpl::OnDownloadSucceeded(const DriverEntry& download,
                                         const base::FilePath& path) {
  Entry* entry = model_->Get(download.guid);
  if (!entry)
    return;

  HandleCompleteDownload(CompletionType::SUCCEED, download.guid);

  download::Client* client = clients_->GetClient(entry->client);
  if (client)
    client->OnDownloadSucceeded(download.guid, path, download.bytes_downloaded);
}

void ControllerImpl::OnDownloadUpdated(const DriverEntry& download) {
  Entry* entry = model_->Get(download.guid);
  if (!entry)
    return;

  DCHECK_EQ(download.state, DriverEntry::State::IN_PROGRESS);
  download::Client* client = clients_->GetClient(entry->client);
  if (client)
    client->OnDownloadUpdated(download.guid, download.bytes_downloaded);
}

void ControllerImpl::OnModelReady(bool success) {
  DCHECK(!startup_status_.model_ok.has_value());
  startup_status_.model_ok = success;
  AttemptToFinalizeSetup();
}

void ControllerImpl::OnItemAdded(bool success,
                                 DownloadClient client,
                                 const std::string& guid) {
  // If the StartCallback doesn't exist, we already notified the Client about
  // this item.  That means something went wrong, so stop here.
  if (start_callbacks_.find(guid) == start_callbacks_.end())
    return;

  if (!success) {
    HandleStartDownloadResponse(client, guid,
                                DownloadParams::StartResult::INTERNAL_ERROR);
    return;
  }

  HandleStartDownloadResponse(client, guid,
                              DownloadParams::StartResult::ACCEPTED);

  Entry* entry = model_->Get(guid);
  DCHECK(entry);
  DCHECK_EQ(Entry::State::NEW, entry->state);
  TransitTo(entry, Entry::State::AVAILABLE, model_.get());

  ActivateMoreDownloads();

  // TODO(dtrainor): Make sure we're running all of the downloads we care about.
}

void ControllerImpl::OnItemUpdated(bool success,
                                   DownloadClient client,
                                   const std::string& guid) {
  // TODO(dtrainor): Fail and clean up the download if necessary.
}

void ControllerImpl::OnItemRemoved(bool success,
                                   DownloadClient client,
                                   const std::string& guid) {
  // TODO(dtrainor): Fail and clean up the download if necessary.
}

void ControllerImpl::OnDeviceStatusChanged(const DeviceStatus& device_status) {
  UpdateDriverStates();
  ActivateMoreDownloads();
}

void ControllerImpl::AttemptToFinalizeSetup() {
  if (!startup_status_.Complete())
    return;

  stats::LogControllerStartupStatus(startup_status_);
  if (!startup_status_.Ok()) {
    // TODO(dtrainor): Recover here.  Try to clean up any disk state and, if
    // possible, any DownloadDriver data and continue with initialization?
    return;
  }

  device_status_listener_->Start(this);
  CancelOrphanedRequests();
  ResolveInitialRequestStates();
  UpdateDriverStates();
  PullCurrentRequestStatus();

  // TODO(dtrainor): Post this so that the initialization step is finalized
  // before Clients can take action.
  NotifyClientsOfStartup();

  // Pull the initial straw if active downloads haven't reach maximum.
  ActivateMoreDownloads();
}

void ControllerImpl::CancelOrphanedRequests() {
  auto entries = model_->PeekEntries();

  std::vector<std::string> guids_to_remove;
  for (auto* entry : entries) {
    if (!clients_->GetClient(entry->client))
      guids_to_remove.push_back(entry->guid);
  }

  for (const auto& guid : guids_to_remove) {
    model_->Remove(guid);

    // Delete the file before removing the download item.
    driver_->DeleteFile(guid);
    driver_->Remove(guid);
  }
}

void ControllerImpl::ResolveInitialRequestStates() {
  // TODO(dtrainor): Implement.
}

void ControllerImpl::UpdateDriverStates() {
  DCHECK(startup_status_.Complete());

  for (auto* const entry : model_->PeekEntries())
    UpdateDriverState(*entry);
}

void ControllerImpl::UpdateDriverState(const Entry& entry) {
  base::Optional<DriverEntry> driver_entry = driver_->Find(entry.guid);

  bool meets_device_criteria = device_status_listener_->CurrentDeviceStatus()
                                   .MeetsCondition(entry.scheduling_params)
                                   .MeetsRequirements();
  switch (entry.state) {
    case Entry::State::ACTIVE:
      if (!meets_device_criteria) {
        driver_->Pause(entry.guid);
        break;
      }
      // Start or resume the download if it should be running.
      if (!driver_entry.has_value()) {
        driver_->Start(entry.request_params, entry.guid,
                       NO_TRAFFIC_ANNOTATION_YET);
        break;
      }
      if (driver_entry->state != DriverEntry::State::IN_PROGRESS) {
        driver_->Resume(entry.guid);
      }
      break;
    case Entry::State::PAUSED:
      // Pause the in progress downloads that should not be running.
      if (driver_entry.has_value() &&
          driver_entry->state == DriverEntry::State::IN_PROGRESS) {
        driver_->Pause(entry.guid);
      }
      break;
    // Fall through.
    case Entry::State::AVAILABLE:
    case Entry::State::NEW:
    case Entry::State::COMPLETE:
    case Entry::State::WATCHDOG:
      // Only active or paused entry can have driver entry.
      if (driver_entry.has_value())
        driver_->Remove(entry.guid);
      break;
    default:
      NOTREACHED();
  }
}

void ControllerImpl::PullCurrentRequestStatus() {
  // TODO(dtrainor): Implement.
}

void ControllerImpl::NotifyClientsOfStartup() {
  auto categorized = util::MapEntriesToClients(clients_->GetRegisteredClients(),
                                               model_->PeekEntries());

  for (auto client_id : clients_->GetRegisteredClients()) {
    clients_->GetClient(client_id)->OnServiceInitialized(
        categorized[client_id]);
  }
}

void ControllerImpl::HandleStartDownloadResponse(
    DownloadClient client,
    const std::string& guid,
    DownloadParams::StartResult result) {
  auto callback = start_callbacks_[guid];
  start_callbacks_.erase(guid);
  HandleStartDownloadResponse(client, guid, result, callback);
}

void ControllerImpl::HandleStartDownloadResponse(
    DownloadClient client,
    const std::string& guid,
    DownloadParams::StartResult result,
    const DownloadParams::StartCallback& callback) {
  stats::LogStartDownloadResult(client, result);

  if (result != DownloadParams::StartResult::ACCEPTED) {
    // TODO(dtrainor): Clean up any download state.
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(callback, guid, result));
}

void ControllerImpl::HandleCompleteDownload(CompletionType type,
                                            const std::string& guid) {
  Entry* entry = model_->Get(guid);
  DCHECK(entry);
  stats::LogDownloadCompletion(type);
  driver_->Remove(guid);

  if (entry->state == Entry::State::COMPLETE ||
      entry->state == Entry::State::WATCHDOG) {
    DVLOG(1) << "Download is already completed.";
    return;
  }

  TransitTo(entry, Entry::State::COMPLETE, model_.get());
  ActivateMoreDownloads();
}

void ControllerImpl::ActivateMoreDownloads() {
  // TODO(xingliu): Check the configuration to throttle downloads.
  Entry* next = scheduler_->Next(
      model_->PeekEntries(), device_status_listener_->CurrentDeviceStatus());

  while (next) {
    DCHECK_EQ(Entry::State::AVAILABLE, next->state);
    TransitTo(next, Entry::State::ACTIVE, model_.get());
    UpdateDriverState(*next);
    next = scheduler_->Next(model_->PeekEntries(),
                            device_status_listener_->CurrentDeviceStatus());
  }
  scheduler_->Reschedule(model_->PeekEntries());
}

}  // namespace download
