// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {
class GeneratePageBundleRequest;
class GetOperationRequest;

class PrefetchNetworkRequestFactory {
 public:
  virtual ~PrefetchNetworkRequestFactory() = default;

  // Creates a new GeneratePageBundle request, retaining ownership.
  virtual void MakeGeneratePageBundleRequest(
      const std::vector<std::string>& prefetch_urls,
      const std::string& gcm_registration_id,
      const PrefetchRequestFinishedCallback& callback) = 0;
  // Gets the current GeneratePageBundleRequest.  After |callback| is executed,
  // this will return |nullptr|.
  virtual GeneratePageBundleRequest* CurrentGeneratePageBundleRequest() = 0;
  // Cancels the current GeneratePageBundleRequest, if any.
  virtual void CancelCurrentGeneratePageBundleRequest() = 0;

  // Creates a new GetOperationRequest, retaining ownership.
  virtual void MakeGetOperationRequest(
      const std::string& operation_name,
      const PrefetchRequestFinishedCallback& callback) = 0;
  // Returns the current GetOperationRequest with the given name, if any.
  virtual GetOperationRequest* FindGetOperationRequestByName(
      const std::string& operation_name) = 0;
  // Cancels the current GetOperationRequest with the given name, if any.
  virtual void CancelGetOperationRequestByName(
      const std::string& operation_name) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_H_
