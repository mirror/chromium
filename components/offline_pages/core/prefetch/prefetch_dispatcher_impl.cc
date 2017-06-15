// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_dispatcher_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_event_logger.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/add_unique_urls_task.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {
void DeleteBackgroundTaskHelper(
    std::unique_ptr<PrefetchDispatcher::ScopedBackgroundTask> task) {
  task.reset();
}
}  // namespace

PrefetchDispatcherImpl::NetworkParams::NetworkParams(
    net::URLRequestContextGetter* request_context,
    const version_info::Channel& channel,
    const std::string& user_agent)
    : request_context(request_context),
      channel(channel),
      user_agent(user_agent) {}

PrefetchDispatcherImpl::NetworkParams::NetworkParams(NetworkParams&& other) =
    default;

PrefetchDispatcherImpl::NetworkParams::~NetworkParams() = default;

PrefetchDispatcherImpl::PrefetchDispatcherImpl(NetworkParams network_params)
    : network_params_(std::move(network_params)), weak_factory_(this) {}

PrefetchDispatcherImpl::~PrefetchDispatcherImpl() = default;

void PrefetchDispatcherImpl::SetService(PrefetchService* service) {
  CHECK(service);
  service_ = service;
}

void PrefetchDispatcherImpl::AddCandidatePrefetchURLs(
    const std::vector<PrefetchURL>& prefetch_urls) {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  std::unique_ptr<Task> add_task = base::MakeUnique<AddUniqueUrlsTask>(
      service_->GetPrefetchStore(), prefetch_urls);
  task_queue_.AddTask(std::move(add_task));

  // TODO(dewittj): Remove in favor of real task-based processing.
  service_->GetPrefetchGCMHandler()->GetGCMToken(
      base::Bind(&PrefetchDispatcherImpl::GotRegistrationId,
                 weak_factory_.GetWeakPtr(), prefetch_urls));
}

void PrefetchDispatcherImpl::RemoveAllUnprocessedPrefetchURLs(
    const std::string& name_space) {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  NOTIMPLEMENTED();
}

void PrefetchDispatcherImpl::RemovePrefetchURLsByClientId(
    const ClientId& client_id) {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  NOTIMPLEMENTED();
}

void PrefetchDispatcherImpl::BeginBackgroundTask(
    std::unique_ptr<ScopedBackgroundTask> task) {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  task_ = std::move(task);
}

void PrefetchDispatcherImpl::StopBackgroundTask() {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  DisposeTask();
}

void PrefetchDispatcherImpl::RequestFinishBackgroundTaskForTest() {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  DisposeTask();
}

void PrefetchDispatcherImpl::DisposeTask() {
  DCHECK(task_);
  // Delay the deletion till the caller finishes.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&DeleteBackgroundTaskHelper, base::Passed(std::move(task_))));
}

void PrefetchDispatcherImpl::GCMOperationCompletedMessageReceived(
    const std::string& operation_name) {
  if (!IsPrefetchingOfflinePagesEnabled())
    return;

  // TODO(dewittj): Remove and replace with a task-based approach using the
  // PrefetchStore.
  get_operation_request_ = base::MakeUnique<GetOperationRequest>(
      operation_name, network_params_.channel,
      network_params_.request_context.get(),
      base::Bind(&PrefetchDispatcherImpl::DonePrefetchRequest,
                 weak_factory_.GetWeakPtr(), "GetOperationRequest"));
}

void PrefetchDispatcherImpl::GotRegistrationId(
    const std::vector<PrefetchURL>& prefetch_urls,
    const std::string& id,
    instance_id::InstanceID::Result result) {
  std::vector<std::string> url_strings;
  for (auto& prefetch_url : prefetch_urls) {
    url_strings.push_back(prefetch_url.url.spec());
  }

  page_bundle_request_ = base::MakeUnique<GeneratePageBundleRequest>(
      network_params_.user_agent, id, 20 * 1024 * 1024, url_strings,
      network_params_.channel, network_params_.request_context.get(),
      base::Bind(&PrefetchDispatcherImpl::DonePrefetchRequest,
                 weak_factory_.GetWeakPtr(), "GeneratePageBundleRequest"));
}

void PrefetchDispatcherImpl::DonePrefetchRequest(
    const std::string& request_name,
    PrefetchRequestStatus status,
    const std::string& operation_name,
    const std::vector<RenderPageInfo>& pages) {
  service_->GetLogger()->RecordActivity(
      "Finished " + request_name + " for operation: " + operation_name +
      " with status: " + std::to_string(static_cast<int>(status)) +
      "; included " + std::to_string(pages.size()) + " pages in result.");
  for (const RenderPageInfo& page : pages) {
    service_->GetLogger()->RecordActivity(
        "Response for page: " + page.url +
        "; status=" + std::to_string(static_cast<int>(page.status)));
  }
}

}  // namespace offline_pages
