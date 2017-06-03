// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/controller_impl.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/internal/client_set.h"
#include "components/download/internal/config.h"
#include "components/download/internal/entry.h"
#include "components/download/internal/entry_utils.h"
#include "components/download/internal/file_monitor.h"
#include "components/download/internal/model.h"
#include "components/download/internal/stats.h"
#include "components/download/public/client.h"

namespace download {

ControllerImpl::ControllerImpl(
    std::unique_ptr<ClientSet> clients,
    std::unique_ptr<Configuration> config,
    std::unique_ptr<DownloadDriver> driver,
    std::unique_ptr<Model> model,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const base::FilePath& dir)
    : clients_(std::move(clients)),
      config_(std::move(config)),
      file_dir_(dir),
      driver_(std::move(driver)),
      model_(std::move(model)),
      file_monitor_(
          base::MakeUnique<FileMonitor>(file_dir_,
                                        background_task_runner,
                                        config_->file_keep_alive_time)),
      weak_factory_(this) {}

ControllerImpl::~ControllerImpl() = default;

void ControllerImpl::Initialize() {
  DCHECK(!startup_status_.Complete());

  driver_->Initialize(this);
  model_->Initialize(this);
}

const StartupStatus& ControllerImpl::GetStartupStatus() {
  return startup_status_;
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
  Entry entry(params);
  entry.target_file_path = file_dir_.AppendASCII(params.guid);
  model_->Add(entry);
}

void ControllerImpl::PauseDownload(const std::string& guid) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  DCHECK(entry);

  if (entry->state == Entry::State::PAUSED ||
      entry->state == Entry::State::COMPLETE ||
      entry->state == Entry::State::WATCHDOG) {
    return;
  }

  // TODO(dtrainor): Pause the download.
}

void ControllerImpl::ResumeDownload(const std::string& guid) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  DCHECK(entry);

  if (entry->state != Entry::State::PAUSED)
    return;

  // TODO(dtrainor): Resume the download.
}

void ControllerImpl::CancelDownload(const std::string& guid) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  DCHECK(entry);

  if (entry->state == Entry::State::NEW) {
    // Check if we're currently trying to add the download.
    DCHECK(start_callbacks_.find(entry->guid) != start_callbacks_.end());
    HandleStartDownloadResponse(entry->client, entry->guid,
                                DownloadParams::StartResult::CLIENT_CANCELLED);
  }

  // TODO(dtrainor): Cancel the download.
}

void ControllerImpl::ChangeDownloadCriteria(const std::string& guid,
                                            const SchedulingParams& params) {
  DCHECK(startup_status_.Ok());

  auto* entry = model_->Get(guid);
  DCHECK(entry);

  // TODO(dtrainor): Change the criteria of the download.
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

void ControllerImpl::OnDownloadCreated(const DriverEntry& download) {}

void ControllerImpl::OnDownloadFailed(const DriverEntry& download, int reason) {
  auto* entry = model_->Get(download.guid);
  DCHECK(entry);
  entry->completion_time = download.completion_time;
  entry->state = Entry::State::COMPLETE;
  model_->Update(*entry);
}

void ControllerImpl::OnDownloadSucceeded(const DriverEntry& download,
                                         const base::FilePath& path) {
  auto* entry = model_->Get(download.guid);
  DCHECK(entry);
  entry->completion_time = download.completion_time;
  entry->state = Entry::State::COMPLETE;
  model_->Update(*entry);
}

void ControllerImpl::OnDownloadUpdated(const DriverEntry& download) {}

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

  auto* entry = model_->Get(guid);
  DCHECK(entry);
  DCHECK_EQ(Entry::State::NEW, entry->state);

  // TODO(dtrainor): Make sure we're running all of the downloads we care about.
}

void ControllerImpl::OnItemUpdated(bool success,
                                   DownloadClient client_id,
                                   const std::string& guid) {
  if (!success) {
    // TODO(shaktisahu): Log UMA?
    return;
  }

  auto* entry = model_->Get(guid);
  if (entry->state == Entry::State::COMPLETE) {
    base::Optional<DriverEntry> driver_entry = driver_->Find(guid);
    DCHECK(driver_entry.has_value());

    driver_->Remove(guid);
    auto* client = clients_->GetClient(client_id);
    DCHECK(client);

    if (driver_entry.value().state == DriverEntry::State::COMPLETE) {
      client->OnDownloadSucceeded(entry->guid, entry->target_file_path,
                                  driver_entry.value().bytes_downloaded);
    } else {
      client->OnDownloadFailed(entry->guid);
    }
  }
}

void ControllerImpl::OnItemRemoved(bool success,
                                   DownloadClient client,
                                   const std::string& guid) {
  base::Optional<DriverEntry> driver_entry = driver_->Find(guid);
  if (success && driver_entry.has_value()) {
    driver_->Remove(guid);
  }
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

  file_monitor_->CancelOrphanedRequests(
      clients_.get(), model_->PeekEntries(),
      base::Bind(&ControllerImpl::RemoveEntriesFromModel,
                 weak_factory_.GetWeakPtr()));

  std::vector<DriverEntry> driver_entries;
  driver_->GetDriverEntries(driver_entries, model_->PeekEntries());
  file_monitor_->RemoveUnassociatedFiles(model_->PeekEntries(), driver_entries);

  ResolveInitialRequestStates();
  PullCurrentRequestStatus();

  // TODO(dtrainor): Post this so that the initialization step is finalized
  // before Clients can take action.
  NotifyClientsOfStartup();
}

void ControllerImpl::ResolveInitialRequestStates() {
  // TODO(dtrainor): Implement.
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

void ControllerImpl::RemoveEntriesFromModel(
    const std::vector<Entry*>& entries) {
  for (auto* entry : entries) {
    model_->Remove(entry->guid);
  }
}

}  // namespace download
