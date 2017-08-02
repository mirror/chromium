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

// DeviceCapabilityManager sends requests to the back-end which enable/disable
// device capabilities and finds devices which contain those capabilities. Here,
// the term "capability" refers to the ability of a device to use a given
// feature.
class DeviceCapabilityManager {
 public:
  enum class Capability { CAPABILITY_UNLOCK_KEY };

  DeviceCapabilityManager(CryptAuthClientFactory* cryptauth_client_factory);

  ~DeviceCapabilityManager();

  // Enables or disables |capability| for the device corresponding to
  // |public_key|. In error cases, |error_callback| is invoked with an error
  // string.
  void SetCapabilityForDevice(
      const std::string& public_key,
      Capability capability,
      bool enabled,
      const base::Closure& success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

  // Fetches metadata about the device which are eligible for |capability|. In
  // error cases, |error_callback| is invoked with an error string.
  void FindEligibleDevices(
      Capability capability,
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          success_callback,
      const base::Callback<void(const std::string&)>& error_callback);

 private:
  enum class RequestType { SET_CAPABILITY, FIND_ELIGIBLE_DEVICES };

  struct Request {
    Request();

    // Used for SET_CAPABILITY Requests.
    Request(RequestType request_type,
            Capability capability,
            std::string public_key,
            bool enabled,
            const base::Closure& set_capability_callback,
            const base::Callback<void(const std::string&)>& error_callback);

    // Used for FIND_ELIGIBLE_DEVICES Requests.
    Request(RequestType request_type,
            Capability capability,
            const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                      const std::vector<IneligibleDevice>&)>&
                find_eligible_devices_callback_,
            const base::Callback<void(const std::string&)>& error_callback);

    ~Request();

    // Defined for every request.
    RequestType request_type_;
    std::string public_key_;
    base::Callback<void(const std::string&)> error_callback_;
    Capability capability_;

    // Defined if |request_type_| is SET_CAPABILITY; otherwise, unused.
    base::Closure set_capability_callback_;
    bool enabled_;

    // Defined if |request_type_| is FIND_ELIGIBLE_DEVICES; otherwise, unused.
    base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                        const std::vector<IneligibleDevice>&)>
        find_eligible_devices_callback_;
  };

  void SetUnlockKeyCapability();

  void ProcessFindEligibleUnlockKeys();

  void FindEligibleUnlockDevices();

  void ProcessSetCapabilityRequest();

  void OnToggleEasyUnlockResponse(const ToggleEasyUnlockResponse& response);

  void OnFindEligibleUnlockDevicesResponse(
      const FindEligibleUnlockDevicesResponse& response);

  void OnErrorResponse(const std::string& response);

  void ProcessRequestQueue();
  std::unique_ptr<Request> current_request_;
  std::queue<std::unique_ptr<Request>> pending_requests_;

  std::unique_ptr<CryptAuthClient> current_cryptauth_client_;
  CryptAuthClientFactory* crypt_auth_client_factory_;
  base::WeakPtrFactory<DeviceCapabilityManager> weak_ptr_factory_;
};
}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_H_
