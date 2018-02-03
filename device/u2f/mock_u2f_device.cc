// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/mock_u2f_device.h"

#include <utility>

#include "components/apdu/apdu_response.h"
#include "device/u2f/u2f_response_test_data.h"

namespace device {

MockU2fDevice::MockU2fDevice() : weak_factory_(this) {}

MockU2fDevice::~MockU2fDevice() = default;

void MockU2fDevice::TryWink(WinkCallback cb) {
  TryWinkRef(cb);
}

void MockU2fDevice::DeviceTransact(std::unique_ptr<apdu::APDUCommand> command,
                                   DeviceCallback cb) {
  DeviceTransactPtr(command.get(), cb);
}

// static
void MockU2fDevice::NotSatisfied(apdu::APDUCommand* cmd, DeviceCallback& cb) {
  std::move(cb).Run(
      true, std::make_unique<apdu::APDUResponse>(
                std::vector<uint8_t>(),
                apdu::APDUResponse::Status::SW_CONDITIONS_NOT_SATISFIED));
}

// static
void MockU2fDevice::WrongData(apdu::APDUCommand* cmd, DeviceCallback& cb) {
  std::move(cb).Run(true, std::make_unique<apdu::APDUResponse>(
                              std::vector<uint8_t>(),
                              apdu::APDUResponse::Status::SW_WRONG_DATA));
}

// static
void MockU2fDevice::NoErrorSign(apdu::APDUCommand* cmd, DeviceCallback& cb) {
  std::move(cb).Run(true, std::make_unique<apdu::APDUResponse>(
                              std::vector<uint8_t>(
                                  std::begin(test_data::kTestU2fSignResponse),
                                  std::end(test_data::kTestU2fSignResponse)),
                              apdu::APDUResponse::Status::SW_NO_ERROR));
}

// static
void MockU2fDevice::NoErrorRegister(apdu::APDUCommand* cmd,
                                    DeviceCallback& cb) {
  std::move(cb).Run(
      true,
      std::make_unique<apdu::APDUResponse>(
          std::vector<uint8_t>(std::begin(test_data::kTestU2fRegisterResponse),
                               std::end(test_data::kTestU2fRegisterResponse)),
          apdu::APDUResponse::Status::SW_NO_ERROR));
}

// static
void MockU2fDevice::SignWithCorruptedResponse(apdu::APDUCommand* cmd,
                                              DeviceCallback& cb) {
  std::move(cb).Run(
      true, std::make_unique<apdu::APDUResponse>(
                std::vector<uint8_t>(
                    std::begin(test_data::kTestCorruptedU2fSignResponse),
                    std::end(test_data::kTestCorruptedU2fSignResponse)),
                apdu::APDUResponse::Status::SW_NO_ERROR));
}

// static
void MockU2fDevice::WinkDoNothing(WinkCallback& cb) {
  std::move(cb).Run();
}

base::WeakPtr<U2fDevice> MockU2fDevice::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace device
