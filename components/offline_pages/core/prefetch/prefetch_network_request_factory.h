// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_context_getter.h"

class GURL;

namespace offline_pages {
class GeneratePageBundleRequest;
class GetOperationRequest;

class PrefetchNetworkRequestFactory
    : public PrefetchDispatcher::NetworkRequestFactory {
 public:
  PrefetchNetworkRequestFactory(net::URLRequestContextGetter* request_context,
                                version_info::Channel channel,
                                const std::string& user_agent);

  ~PrefetchNetworkRequestFactory() override;

  std::unique_ptr<GeneratePageBundleRequest> MakeGeneratePageBundleRequest(
      const std::vector<GURL>& prefetch_urls,
      const std::string& gcm_registration_id,
      const PrefetchRequestFinishedCallback& callback) override;

  std::unique_ptr<GetOperationRequest> MakeGetOperationRequest(
      const std::string& operation_name,
      const PrefetchRequestFinishedCallback& callback) override;

 private:
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  version_info::Channel channel_;
  std::string user_agent_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_H_
