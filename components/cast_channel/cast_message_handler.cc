// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_message_handler.h"

#include <tuple>

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/default_tick_clock.h"
#include "components/cast_channel/cast_socket_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace cast_channel {

namespace {

constexpr net::NetworkTrafficAnnotationTag kVirtualConnectionTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("cast_message_handler", R"(
        semantics {
          sender: "Cast Message Handler"
          description:
            "A virtual connection request sent to a Cast device."
          trigger:
            "A Cast protocol or application-level message is being sent to a "
            "Cast device."
          data:
            "A serialized protobuf message containing request. No user data."
          destination: OTHER
          destination_other:
            "Data will be sent to a Cast device in local network."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be disabled, but it would not be sent if user "
            "does not connect a Cast device to the local network."
          policy_exception_justification: "Not implemented."
        })");

constexpr net::NetworkTrafficAnnotationTag kMessageTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("cast_message_handler", R"(
        semantics {
          sender: "Cast Message Handler"
          description:
            "A Cast protocol or application-level message sent to a Cast "
            "device."
          trigger:
            "There are several possible triggers: "
            "- User gesture from using Cast functionality, "
            "- A webpage using the Presentation API, "
            "- Cast device discovery internal logic."
          data:
            "A serialized protobuf message containing. Application-level "
            "messages may contain application-specific data."
          destination: OTHER
          destination_other:
            "Data will be sent to a Cast device in local network."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This request cannot be disabled, but it would not be sent if user "
            "does not connect a Cast device to the local network."
          policy_exception_justification: "Not implemented."
        })");

}  // namespace

GetAppAvailabilityRequest::GetAppAvailabilityRequest(
    int channel_id,
    const std::string& app_id,
    GetAppAvailabilityCallback callback,
    base::TickClock* clock)
    : channel_id(channel_id),
      app_id(app_id),
      callback(std::move(callback)),
      timeout_timer(clock) {}

GetAppAvailabilityRequest::~GetAppAvailabilityRequest() = default;

VirtualConnection::VirtualConnection(int channel_id,
                                     const std::string& source_id,
                                     const std::string& destination_id)
    : channel_id(channel_id),
      source_id(source_id),
      destination_id(destination_id) {}
VirtualConnection::~VirtualConnection() = default;

bool VirtualConnection::operator<(const VirtualConnection& other) const {
  return std::tie(channel_id, source_id, destination_id) <
         std::tie(other.channel_id, other.source_id, other.destination_id);
}

// static
CastMessageHandler* CastMessageHandler::GetInstance() {
  static CastMessageHandler* instance =
      new CastMessageHandler(CastSocketService::GetInstance());
  return instance;
}

CastMessageHandler::CastMessageHandler(CastSocketService* socket_service)
    : sender_id_(base::StringPrintf("sender-%d", base::RandInt(0, 1000000))),
      socket_service_(socket_service),
      clock_(base::DefaultTickClock::GetInstance()),
      weak_ptr_factory_(this) {
  socket_service_->AddObserver(this);
}

CastMessageHandler::~CastMessageHandler() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  socket_service_->RemoveObserver(this);
}

void CastMessageHandler::RequestAppAvailability(
    CastSocket* socket,
    const std::string& app_id,
    GetAppAvailabilityCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int request_id = NextRequestId();

  DVLOG(2) << __func__ << ", socket_id: " << socket->id()
           << ", app_id: " << app_id << ", request_id: " << request_id;
  CastMessage message =
      CreateGetAppAvailabilityRequest(sender_id_, request_id, app_id);

  auto request = std::make_unique<GetAppAvailabilityRequest>(
      socket->id(), app_id, std::move(callback), clock_);
  request->timeout_timer.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(5),
      base::Bind(&CastMessageHandler::GetAppAvailabilityTimedOut,
                 base::Unretained(this), request_id));
  pending_app_availability_requests_.emplace(request_id, std::move(request));

  SendCastMessage(socket, message);
}

void CastMessageHandler::GetAppAvailabilityTimedOut(int request_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << __func__ << ", request_id: " << request_id;
  auto it = pending_app_availability_requests_.find(request_id);
  if (it == pending_app_availability_requests_.end())
    return;

  std::move(it->second->callback)
      .Run(it->second->app_id, GetAppAvailabilityResult::kUnknown);
  pending_app_availability_requests_.erase(it);
}

void CastMessageHandler::OnError(const CastSocket& socket,
                                 ChannelError error_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::EraseIf(virtual_connections_,
                [&socket](const VirtualConnection& connection) {
                  return connection.channel_id == socket.id();
                });

  auto it = pending_app_availability_requests_.begin();
  while (it != pending_app_availability_requests_.end()) {
    if (it->second->channel_id == socket.id()) {
      std::move(it->second->callback)
          .Run(it->second->app_id, GetAppAvailabilityResult::kUnknown);
      it = pending_app_availability_requests_.erase(it);
    } else {
      ++it;
    }
  }
}

void CastMessageHandler::OnMessage(const CastSocket& socket,
                                   const CastMessage& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(imcheng): Handle other messages.
  DVLOG(2) << __func__ << ", socket_id: " << socket.id()
           << ", message: " << CastMessageToString(message);
  if (!IsReceiverMessage(message) || message.destination_id() != sender_id_)
    return;

  std::unique_ptr<base::Value> payload = GetDictionaryFromCastMessage(message);
  if (!payload)
    return;

  int request_id = 0;
  if (!GetRequestIdFromResponse(*payload, &request_id))
    return;

  auto it = pending_app_availability_requests_.find(request_id);
  if (it != pending_app_availability_requests_.end()) {
    GetAppAvailabilityResult result =
        GetAppAvailabilityResultFromResponse(*payload, it->second->app_id);
    std::move(it->second->callback).Run(it->second->app_id, result);
    pending_app_availability_requests_.erase(it);
  }
}

void CastMessageHandler::SendCastMessage(CastSocket* socket,
                                         const CastMessage& message) {
  // A virtual connection must be opened to the receiver before other messages
  // can be sent.
  VirtualConnection connection(socket->id(), message.source_id(),
                               message.destination_id());
  if (virtual_connections_.find(connection) == virtual_connections_.end()) {
    DVLOG(1) << "Creating VC for channel: " << connection.channel_id
             << ", source: " << connection.source_id
             << ", dest: " << connection.destination_id;
    CastMessage virtual_connection_request = CreateVirtualConnectionRequest(
        message.source_id(), message.destination_id());
    socket->transport()->SendMessage(
        virtual_connection_request,
        base::Bind(&CastMessageHandler::OnMessageSent,
                   weak_ptr_factory_.GetWeakPtr()),
        kVirtualConnectionTrafficAnnotation);

    // We assume the virtual connection request will succeed; otherwise this
    // will eventually self-correct.
    virtual_connections_.insert(connection);
  }
  socket->transport()->SendMessage(
      message,
      base::Bind(&CastMessageHandler::OnMessageSent,
                 weak_ptr_factory_.GetWeakPtr()),
      kMessageTrafficAnnotation);
}

void CastMessageHandler::OnMessageSent(int result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG_IF(2, result < 0) << "SendMessage failed with code: " << result;
}

}  // namespace cast_channel
