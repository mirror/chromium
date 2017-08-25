// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/pollux/assertion_session.h"

#include <iomanip>

#include "base/base64url.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/wire_message.h"
#include "components/proximity_auth/logging/logging.h"
#include "components/proximity_auth/pollux/credential_utils.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace pollux {

namespace {

const char kAdvertisementUUID[] = "0000fe51-0000-1000-8000-00805f9b34fb";

std::string Base64Encode(const std::string& data) {
  std::string encoded_value;
  base::Base64UrlEncode(data, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &encoded_value);

  return encoded_value;
}

std::string ToHex(const std::string& data) {
  std::stringstream ss;
  ss << "0x";
  for (uint8_t byte : data) {
    ss << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<uint64_t>(byte);
  }
  return ss.str();
}

}  // namespace

AssertionSession::AssertionSession(const Parameters& parameters)
    : parameters_(parameters),
      state_(State::NOT_STARTED),
      weak_ptr_factory_(this) {}

AssertionSession::~AssertionSession() {
  StopAdvertisement();

  if (connection_) {
    connection_->RemoveObserver(this);
    connection_->Disconnect();
  }
}

void AssertionSession::Start() {
  PA_LOG(INFO) << "Start assertion session";
  DCHECK(state_ == State::NOT_STARTED);
  // 1. Advertise
  SetState(State::ADVERTISING);

  device::BluetoothAdapterFactory::GetAdapter(base::Bind(
      &AssertionSession::OnAdapterInitialized, weak_ptr_factory_.GetWeakPtr()));
}

void AssertionSession::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AssertionSession::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AssertionSession::SetState(State state) {
  State old_state = state_;
  state_ = state;
  for (auto& observer : observers_) {
    observer.OnStateChanged(old_state, state_);
  }
}

void AssertionSession::OnAdapterInitialized(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  PA_LOG(INFO) << "Got Adapter";
  adapter_ = adapter;

  std::unique_ptr<device::BluetoothAdvertisement::Data> advertisement_data =
      base::MakeUnique<device::BluetoothAdvertisement::Data>(
          device::BluetoothAdvertisement::AdvertisementType::
              ADVERTISEMENT_TYPE_BROADCAST);

  std::unique_ptr<device::BluetoothAdvertisement::UUIDList> uuid_list =
      base::MakeUnique<device::BluetoothAdvertisement::UUIDList>();
  uuid_list->push_back(std::string(kAdvertisementUUID));
  advertisement_data->set_service_uuids(std::move(uuid_list));

  const std::string& eid = parameters_.eid;
  std::vector<uint8_t> data_as_vector(eid.size());
  memcpy(data_as_vector.data(), eid.data(), eid.size());
  std::unique_ptr<device::BluetoothAdvertisement::ServiceData> service_data =
      base::MakeUnique<device::BluetoothAdvertisement::ServiceData>();
  service_data->insert(std::pair<std::string, std::vector<uint8_t>>(
      std::string(kAdvertisementUUID), data_as_vector));
  advertisement_data->set_service_data(std::move(service_data));

  PA_LOG(INFO) << "Advertising EID: "
               << "\n  B64: " << Base64Encode(eid) << "\n  HEX: " << ToHex(eid);
  adapter_->RegisterAdvertisement(
      std::move(advertisement_data),
      base::Bind(&AssertionSession::OnAdvertisementRegistered,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&AssertionSession::OnAdvertisementError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void AssertionSession::OnAdvertisementRegistered(
    scoped_refptr<device::BluetoothAdvertisement> advertisement) {
  PA_LOG(INFO) << "Successfully advertised...";
  advertisement_ = advertisement;
  std::string remote_eid = credential_utils::DeriveRemoteEid(
      parameters_.session_key, parameters_.eid);

  PA_LOG(INFO) << "Scanning for remote EID: "
               << "\n  B64: " << Base64Encode(remote_eid)
               << "\n  HEX: " << ToHex(remote_eid);

  ble_scanner_.reset(new BleScanner(adapter_, remote_eid));
  ble_scanner_->Find(base::Bind(&AssertionSession::OnConnectionFound,
                                weak_ptr_factory_.GetWeakPtr()));
}

void AssertionSession::StopAdvertisement() {
  if (advertisement_) {
    PA_LOG(INFO) << "Unregistering advertisement";
    advertisement_->Unregister(
        base::Bind(&base::DoNothing),
        base::Bind(&AssertionSession::OnUnregisterAdvertisementError,
                   weak_ptr_factory_.GetWeakPtr()));
    advertisement_ = nullptr;
  }
}

void AssertionSession::OnAdvertisementError(
    device::BluetoothAdvertisement::ErrorCode error_code) {
  PA_LOG(ERROR) << "Error advertising: " << static_cast<int>(error_code);
  SetState(State::ERROR);
}

void AssertionSession::OnUnregisterAdvertisementError(
    device::BluetoothAdvertisement::ErrorCode error_code) {
  PA_LOG(ERROR) << "Error unregistering advertisement: "
                << static_cast<int>(error_code);
}

void AssertionSession::OnConnectionFound(
    std::unique_ptr<cryptauth::Connection> connection) {
  PA_LOG(INFO) << "CONNECTION FOUND!";
  SetState(State::CONNECTED);

  connection_ = std::move(connection);
  connection_->AddObserver(this);
  StopAdvertisement();
  ble_scanner_.reset();

  // Send the challenge.
  std::unique_ptr<cryptauth::WireMessage> wire_message(
      new cryptauth::WireMessage(parameters_.challenge, "pollux"));
  connection_->SendMessage(std::move(wire_message));
}

void AssertionSession::OnConnectionStatusChanged(
    cryptauth::Connection* connection,
    cryptauth::Connection::Status old_status,
    cryptauth::Connection::Status new_status) {
  if (state_ == State::CONNECTED &&
      new_status == cryptauth::Connection::Status::DISCONNECTED) {
    PA_LOG(WARNING) << "Unexpected disconnect.";
    SetState(State::UNEXPECTED_DISCONNECT);
    connection_.reset();
  }
}

void AssertionSession::OnMessageReceived(
    const cryptauth::Connection& connection,
    const cryptauth::WireMessage& message) {
  PA_LOG(INFO) << "Received message:"
               << "\n  feature: " << message.feature()
               << "\n  payload: " << message.payload();

  SetState(State::COMPLETED_WITH_ASSERTION);
  for (auto& observer : observers_) {
    observer.OnGotAssertion(message.payload());
  }

  if (connection_) {
    connection_->RemoveObserver(this);
    connection_->Disconnect();
  }
}

void AssertionSession::OnSendCompleted(const cryptauth::Connection& connection,
                                       const cryptauth::WireMessage& message,
                                       bool success) {
  PA_LOG(INFO) << "Sent message: " << success;
}

}  // namespace pollux
