// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_network_request_factory_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"

namespace {
constexpr int kMaxBundleSizeBytes = 20 * 1024 * 1024;  // 20 MB

// Max concurrent outstanding requests. If more requests asked to be created,
// the requests are silently not created (considered failed). This is used
// as emergency limit that should rarely be encountered in normal operations.
const int kMaxConcurrentRequests = 10;

// Static in-memory count of outstanding network requests.
static int concurrent_request_count = 0;

bool AddConcurrentRequest() {
  if (concurrent_request_count >= kMaxConcurrentRequests)
    return false;
  ++concurrent_request_count;
  return true;
}

void ReleaseConcurrentRequest() {
  DCHECK(concurrent_request_count > 0);
  --concurrent_request_count;
}

// In-memory request id, incremented for each new GeneratePageBundleRequest.
static uint64_t GetNextRequestId() {
  static uint64_t request_id = 0;
  return ++request_id;
}
}  // namespace

namespace offline_pages {

PrefetchNetworkRequestFactoryImpl::PrefetchNetworkRequestFactoryImpl(
    net::URLRequestContextGetter* request_context,
    version_info::Channel channel,
    const std::string& user_agent)
    : request_context_(request_context),
      channel_(channel),
      user_agent_(user_agent),
      weak_factory_(this) {}

PrefetchNetworkRequestFactoryImpl::~PrefetchNetworkRequestFactoryImpl() =
    default;

bool PrefetchNetworkRequestFactoryImpl::HasOutstandingRequests() {
  return !(generate_page_bundle_requests_.empty() &&
           get_operation_requests_.empty());
}

void PrefetchNetworkRequestFactoryImpl::MakeGeneratePageBundleRequest(
    const std::vector<std::string>& url_strings,
    const std::string& gcm_registration_id,
    const PrefetchRequestFinishedCallback& callback) {
  if (!AddConcurrentRequest())
    return;
  uint64_t request_id = GetNextRequestId();
  generate_page_bundle_requests_[request_id] =
      base::MakeUnique<GeneratePageBundleRequest>(
          user_agent_, gcm_registration_id, kMaxBundleSizeBytes, url_strings,
          channel_, request_context_.get(),
          base::Bind(
              &PrefetchNetworkRequestFactoryImpl::GeneratePageBundleRequestDone,
              weak_factory_.GetWeakPtr(), callback, request_id));
}

bool PrefetchNetworkRequestFactoryImpl::HasGeneratePageBundleRequestForUrl(
    const std::string& url_spec) const {
  for (const auto& request_pair : generate_page_bundle_requests_) {
    const std::set<std::string>& requested_urls =
        request_pair.second->requested_urls();
    if (requested_urls.find(url_spec) != requested_urls.end())
      return true;
  }
  return false;
}

void PrefetchNetworkRequestFactoryImpl::MakeGetOperationRequest(
    const std::string& operation_name,
    const PrefetchRequestFinishedCallback& callback) {
  if (!AddConcurrentRequest())
    return;
  get_operation_requests_[operation_name] =
      base::MakeUnique<GetOperationRequest>(
          operation_name, channel_, request_context_.get(),
          base::Bind(
              &PrefetchNetworkRequestFactoryImpl::GetOperationRequestDone,
              weak_factory_.GetWeakPtr(), callback));
}

void PrefetchNetworkRequestFactoryImpl::GeneratePageBundleRequestDone(
    const PrefetchRequestFinishedCallback& callback,
    uint64_t request_id,
    PrefetchRequestStatus status,
    const std::string& operation_name,
    const std::vector<RenderPageInfo>& pages) {
  callback.Run(status, operation_name, pages);
  generate_page_bundle_requests_.erase(request_id);
  ReleaseConcurrentRequest();
}

void PrefetchNetworkRequestFactoryImpl::GetOperationRequestDone(
    const PrefetchRequestFinishedCallback& callback,
    PrefetchRequestStatus status,
    const std::string& operation_name,
    const std::vector<RenderPageInfo>& pages) {
  callback.Run(status, operation_name, pages);
  get_operation_requests_.erase(operation_name);
  ReleaseConcurrentRequest();
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
