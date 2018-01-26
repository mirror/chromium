// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_FIDO_DEVICE_H_
#define DEVICE_CTAP_FIDO_DEVICE_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "device/ctap/authenticator_get_info_response.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_get_assertion_request_param.h"
#include "device/ctap/ctap_make_credential_request_param.h"
#include "device/ctap/ctap_request_param.h"
#include "device/ctap/u2f_register_param.h"
#include "device/ctap/u2f_sign_param.h"

namespace device {

// Device interface that encapsulates protocol supported by the device as well
// as corresponding transport specific communication with the authenticator.
class FidoDevice {
 public:
  enum class State { kInit, kConnected, kReady, kBusy, kDeviceError };

  // Device response callback for AuthenticatorMakeCredential request,
  // U2F Register request, AuthenticatorReset, and AuthenticatorGetInfo
  // command.
  using DeviceCallback =
      base::OnceCallback<void(CTAPDeviceResponseCode response_code,
                              const std::vector<uint8_t>& response_buffer)>;

  // Callback that converts CTAP request parameter into U2F request parameter
  // spawns a corresponding U2F child request.
  using ConvertToU2FRequestCallback =
      base::OnceCallback<void(std::unique_ptr<CTAPRequestParam> command,
                              DeviceCallback cb)>;

  FidoDevice();
  virtual ~FidoDevice();

  virtual const std::string GetId() const = 0;
  virtual base::WeakPtr<FidoDevice> GetWeakPtr() = 0;

  void MakeCredential(std::unique_ptr<CTAPMakeCredentialRequestParam> command,
                      DeviceCallback cb,
                      ConvertToU2FRequestCallback u2f_callback);
  void GetAssertion(std::unique_ptr<CTAPGetAssertionRequestParam> command,
                    DeviceCallback cb,
                    ConvertToU2FRequestCallback u2f_callback);
  void GetNextAssertion(DeviceCallback cb);
  void Register(std::unique_ptr<U2FRegisterParam> command,
                DeviceCallback callback);
  void Sign(std::unique_ptr<U2FSignParam> command, DeviceCallback callback);
  void GetInfo(DeviceCallback cb);
  void Reset(DeviceCallback cb);
  void Cancel();

  void GetInfoCallback(std::unique_ptr<CTAPRequestParam> command,
                       DeviceCallback callback,
                       CTAPDeviceResponseCode response_code,
                       const std::vector<uint8_t>& response_buffer);

  bool AuthenticatorInfoObtained() const;
  bool SupportsCTAP() const;

 protected:
  // Pure virtual function defined by each device type. Initiates a device
  // communication transaction process.
  virtual void DeviceTransact(std::unique_ptr<CTAPRequestParam> command,
                              DeviceCallback callback) = 0;

  std::tuple<CTAPDeviceResponseCode, std::vector<uint8_t>> DecodeDeviceResponse(
      std::vector<uint8_t> buffer);

  CTAPDeviceResponseCode GetDeviceProcessingError();

  State state_ = State::kInit;
  ProtocolVersion supported_protocol_ = ProtocolVersion::kCtap;
  std::unique_ptr<AuthenticatorGetInfoResponse> device_info_;
  ConvertToU2FRequestCallback u2f_callback_;

  DISALLOW_COPY_AND_ASSIGN(FidoDevice);
};

}  // namespace device

#endif  // DEVICE_CTAP_FIDO_DEVICE_H_
