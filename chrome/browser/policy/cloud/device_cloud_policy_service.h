// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_CLOUD_DEVICE_CLOUD_POLICY_SERVICE_H_
#define CHROME_BROWSER_POLICY_CLOUD_DEVICE_CLOUD_POLICY_SERVICE_H_

#include <memory>

#include "components/policy/core/common/cloud/cloud_policy_client.h"

namespace policy {

class DeviceManagementService;

class DeviceCloudPolicyService : public CloudPolicyClient::Observer {
 public:
  DeviceCloudPolicyService(
      DeviceManagementService* device_management_service,
      scoped_refptr<net::URLRequestContextGetter> system_request_context);
  ~DeviceCloudPolicyService() override;
  void FetchPolicy(const std::string& dm_token);

  void OnPolicyFetched(CloudPolicyClient* client) override;
  void OnRegistrationStateChanged(CloudPolicyClient* client) override;
  void OnClientError(CloudPolicyClient* client) override;
  net::URLRequestContextGetter* system_request_context() {
    return system_request_context_.get();
  }

 private:
  std::unique_ptr<CloudPolicyClient> cloud_policy_client_;
  DeviceManagementService* device_management_service_;
  scoped_refptr<net::URLRequestContextGetter> system_request_context_;
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_CLOUD_DEVICE_CLOUD_POLICY_SERVICE_H_
