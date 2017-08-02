// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_DEVICE_CAPABILITY_MANAGER_H_
#define COMPONENTS_CRYPTAUTH_DEVICE_CAPABILITY_MANAGER_H_

#include <queue>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

// This class is an agent that can turn on capabilities and retrieve lists of
// devices with certain capabilities on behalf of authorized devices.
class DeviceCapabilityManager {
 public:
  enum class Capability { CAPABILITY_UNLOCK_KEY };

  DeviceCapabilityManager(CryptAuthClientFactory* cryptauth_client_factory);

  ~DeviceCapabilityManager();

  // Enables or disables a Capabilitiy |capability| for a remote device with
  // |public_key|.  User must pass a |success_callback| and |error_callback|
  // callback that is executed upon completion.
  void SetCapabilityForDevice(
      const std::string& public_key,
      Capability capability,
      bool enabled,
      const base::Closure& success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

  // Fetches a vector of ExternalDeviceInfo and a vector of IneligibleDevice.
  // User must pass a |success_callback| and |error_callback| callback that is
  // executed upon completion.
  void FindEligibleDevices(
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

 private:
  enum class RequestType { SET_CAPABILITY, FIND_ELIGIBLE_DEVICES };

  struct Request {
    Request();

    // This constructor is used for Requests that toggle a capability.
    Request(RequestType request_type,
            Capability capability,
            std::string public_key,
            bool enabled,
            const base::Closure& set_capability_callback,
            const base::Callback<void(const std::string&)>& error_callback);

    // This constructor is used for Requests that fetch vectors of eligible and
    // ineligle devices.
    Request(RequestType request_type,
            const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                      const std::vector<IneligibleDevice>&)>&
                find_eligible_devices_callback_,
            const base::Callback<void(const std::string&)>& error_callback);

    ~Request();

    // Defined for every request.
    RequestType request_type_;
    std::string public_key_;
    base::Callback<void(const std::string&)> error_callback_;

    // Defined if |request_type_| is SET_CAPABILITY; otherwise, unused.
    Capability capability_;
    base::Closure set_capability_callback_;
    bool enabled_;

    // Defined if |request_type_| is FIND_ELIGIBLE_DEVICES; otherwise, unused.
    base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                        const std::vector<IneligibleDevice>&)>
        find_eligible_devices_callback_;
  };

  void SetUnlockKeyCapability();

  void ProcessFindEligibleUnlockKeys();

  void OnToggleEasyUnlockResponse(const base::Closure& original_callback,
                                  const ToggleEasyUnlockResponse& response);

  void OnFindEligibleUnlockDevicesResponse(
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          original_callback,
      const FindEligibleUnlockDevicesResponse& response);

  void OnErrorResponse(
      const base::Callback<void(const std::string&)>& original_callback,
      const std::string& response);

  void ProcessSetCapabilityRequest();

  void ProcessRequestQueue();
  std::unique_ptr<Request> current_request_;
  std::queue<std::unique_ptr<Request>> pending_requests_;

  std::unique_ptr<CryptAuthClient> current_cryptauth_client_;
  CryptAuthClientFactory* crypt_auth_client_factory_;
  base::WeakPtrFactory<DeviceCapabilityManager> weak_ptr_factory_;
};
}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_H_
