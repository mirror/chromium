// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/device_cloud_policy_service.h"

#include "components/policy/core/common/cloud/device_management_service.h"
#include "net/url_request/url_request_context_getter.h"

namespace policy {
namespace {}

DeviceCloudPolicyService::DeviceCloudPolicyService(
    DeviceManagementService* device_management_service,
    scoped_refptr<net::URLRequestContextGetter> system_request_context)
    : device_management_service_(device_management_service),
      system_request_context_(system_request_context) {
  FetchPolicy("");
}
DeviceCloudPolicyService::~DeviceCloudPolicyService() {}

void DeviceCloudPolicyService::FetchPolicy(const std::string& dm_token) {
  cloud_policy_client_ = std::make_unique<CloudPolicyClient>(
      std::string(), std::string(), device_management_service_,
      system_request_context_, nullptr);
}

void DeviceCloudPolicyService::OnPolicyFetched(CloudPolicyClient* client) {}
void DeviceCloudPolicyService::OnRegistrationStateChanged(
    CloudPolicyClient* client) {}
void DeviceCloudPolicyService::OnClientError(CloudPolicyClient* client) {}

}  // namespace policy
