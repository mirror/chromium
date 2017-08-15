// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_network_request_factory_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"

constexpr int kMaxBundleSizeBytes = 20 * 1024 * 1024;  // 20 MB

namespace offline_pages {

PrefetchNetworkRequestFactoryImpl::PrefetchNetworkRequestFactoryImpl(
    PrefetchDispatcher* dispatcher,
    net::URLRequestContextGetter* request_context,
    version_info::Channel channel,
    const std::string& user_agent)
    : dispatcher_(dispatcher),
      request_context_(request_context),
      channel_(channel),
      user_agent_(user_agent),
      weak_factory_(this) {}

PrefetchNetworkRequestFactoryImpl::~PrefetchNetworkRequestFactoryImpl() =
    default;

void PrefetchNetworkRequestFactoryImpl::MakeGeneratePageBundleRequest(
    const std::vector<std::string>& url_strings,
    const std::string& gcm_registration_id,
    const PrefetchRequestFinishedCallback& callback) {
  auto request = base::MakeUnique<GeneratePageBundleRequest>(
      user_agent_, gcm_registration_id, kMaxBundleSizeBytes, url_strings,
      channel_, request_context_.get(),
      base::Bind(
          &PrefetchNetworkRequestFactoryImpl::GeneratePageBundleRequestDone,
          weak_factory_.GetWeakPtr(), callback));
  generate_page_bundle_request_ = std::move(request);
  if (background_task_ == nullptr)
    background_task_ = dispatcher_->GetBackgroundTask();
}

GeneratePageBundleRequest*
PrefetchNetworkRequestFactoryImpl::CurrentGeneratePageBundleRequest() const {
  return generate_page_bundle_request_.get();
}

void PrefetchNetworkRequestFactoryImpl::MakeGetOperationRequest(
    const std::string& operation_name,
    const PrefetchRequestFinishedCallback& callback) {
  auto request = base::MakeUnique<GetOperationRequest>(
      operation_name, channel_, request_context_.get(),
      base::Bind(&PrefetchNetworkRequestFactoryImpl::GetOperationRequestDone,
                 weak_factory_.GetWeakPtr(), callback));
  get_operation_requests_[operation_name] = std::move(request);
  if (background_task_ == nullptr)
    background_task_ = dispatcher_->GetBackgroundTask();
}

void PrefetchNetworkRequestFactoryImpl::GeneratePageBundleRequestDone(
    const PrefetchRequestFinishedCallback& callback,
    PrefetchRequestStatus status,
    const std::string& operation_name,
    const std::vector<RenderPageInfo>& pages) {
  callback.Run(status, operation_name, pages, background_task_);
  generate_page_bundle_request_.reset();
  if (get_operation_requests_.empty())
    background_task_ = nullptr;
}

void PrefetchNetworkRequestFactoryImpl::GetOperationRequestDone(
    const PrefetchRequestFinishedCallback& callback,
    PrefetchRequestStatus status,
    const std::string& operation_name,
    const std::vector<RenderPageInfo>& pages) {
  callback.Run(status, operation_name, pages, background_task_);
  get_operation_requests_.erase(operation_name);
  if (get_operation_requests_.empty() &&
      generate_page_bundle_request_ == nullptr) {
    background_task_ = nullptr;
  }
}

GetOperationRequest*
PrefetchNetworkRequestFactoryImpl::FindGetOperationRequestByName(
    const std::string& operation_name) const {
  auto iter = get_operation_requests_.find(operation_name);
  if (iter == get_operation_requests_.end())
    return nullptr;

  return iter->second.get();
}

}  // namespace offline_pages
