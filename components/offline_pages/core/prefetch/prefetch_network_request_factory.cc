// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/prefetch/generate_page_bundle_request.h"
#include "components/offline_pages/core/prefetch/get_operation_request.h"

constexpr int kMaxBundleSizeBytes = 20 * 1024 * 1024;  // 20 MB

namespace offline_pages {

PrefetchNetworkRequestFactory::PrefetchNetworkRequestFactory(
    net::URLRequestContextGetter* request_context,
    version_info::Channel channel,
    const std::string& user_agent)
    : request_context_(request_context),
      channel_(channel),
      user_agent_(user_agent) {}

PrefetchNetworkRequestFactory::~PrefetchNetworkRequestFactory() = default;

std::unique_ptr<GeneratePageBundleRequest>
PrefetchNetworkRequestFactory::MakeGeneratePageBundleRequest(
    const std::vector<GURL>& urls,
    const std::string& gcm_registration_id,
    const PrefetchRequestFinishedCallback& callback) {
  std::vector<std::string> url_strings;
  for (auto& url : urls) {
    url_strings.push_back(url.spec());
  }

  return base::WrapUnique(new GeneratePageBundleRequest(
      user_agent_, gcm_registration_id, kMaxBundleSizeBytes, url_strings,
      channel_, request_context_.get(), callback));
}

std::unique_ptr<GetOperationRequest>
PrefetchNetworkRequestFactory::MakeGetOperationRequest(
    const std::string& operation_name,
    const PrefetchRequestFinishedCallback& callback) {
  return base::WrapUnique(new GetOperationRequest(
      operation_name, channel_, request_context_.get(), callback));
}

}  // namespace offline_pages
