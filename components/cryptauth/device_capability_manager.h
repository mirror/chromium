// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_DEVICE_CAPABILITY_MANAGER_H_
#define COMPONENTS_CRYPTAUTH_DEVICE_CAPABILITY_MANAGER_H_

#include <queue>
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {
// This class manages device capabilities.
class DeviceCapabilityManager {
 public:
  enum class Capability {
    CAPABILITY_UNLOCK_KEY,
    CAPABILITY_READ_UNLOCK_DEVICES
  };

  DeviceCapabilityManager(CryptAuthClientFactory* cryptauth_client_factory);

  ~DeviceCapabilityManager();

  void SetCapabilityForDevice(
      const std::string& public_key,
      Capability capability,
      bool enabled,
      const base::Closure& success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

  void FindEligibleDevices(
      Capability capability,
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

 private:
  enum class RequestType { SET_CAPABILITY, FIND_ELIGIBLE_DEVICES };

  struct Request {
   public:
    Request();

    Request(RequestType request_type,
            Capability capability,
            std::string public_key,
            bool enabled,
            const base::Closure& success_callback,
            const base::Callback<void(const std::string&)>& error_callback);

    Request(RequestType request_type,
            Capability capability,
            const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                      const std::vector<IneligibleDevice>&)>&
                success_callback,
            const base::Callback<void(const std::string&)>& error_callback);

    ~Request();

    explicit Request(const Request& request);

    RequestType request_type_;
    Capability capability_;
    std::string public_key_;
    bool enabled_;
    base::Closure success_callback_;
    base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                        const std::vector<IneligibleDevice>&)>
        success_callback_vector_arg_;
    base::Callback<void(const std::string&)> error_callback_;
  };

  void ToggleEasyUnlock(const std::string& public_key,
                        bool enabled,
                        const base::Closure& success_callback,
                        const CryptAuthClient::ErrorCallback& error_callback);

  void FindEligibleForUnlockDevices(
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

  void OnToggleEasyUnlockResponse(const base::Closure& original_callback,
                                  const ToggleEasyUnlockResponse& response);

  void OnEligibleDevicesResponse(
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          original_callback,
      const FindEligibleUnlockDevicesResponse& response);

  void OnErrorResponse(
      const base::Callback<void(const std::string&)>& original_callback,
      const std::string& response);

  void InvokeNextRequest();
  void PingScheduler();

  CryptAuthClientFactory* crypt_auth_client_factory_;
  bool in_progress_;
  std::queue<Request> scheduler_;
  base::WeakPtrFactory<DeviceCapabilityManager> weak_ptr_factory_;
};
}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_H_
