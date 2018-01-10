// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_message_handler.h"

#include <tuple>

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "components/cast_channel/cast_socket_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace media_router {

GetAppAvailabilityRequest::GetAppAvailabilityRequest(
    const std::string& app_id,
    GetAppAvailabilityCallback callback)
    : app_id(app_id), callback(std::move(callback)) {}

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

CastMessageHandler::CastMessageHandler(
    cast_channel::CastSocketService* socket_service)
    : sender_id_(base::StringPrintf("sender-%d", base::RandInt(0, 1000000))),
      socket_service_(socket_service) {
  socket_service_->AddObserver(this);
}

CastMessageHandler::~CastMessageHandler() {
  socket_service_->RemoveObserver(this);
}

void CastMessageHandler::RequestAppAvailability(
    cast_channel::CastSocket* socket,
    const std::string& app_id,
    GetAppAvailabilityCallback callback) {
  int request_id = NextRequestId();

  pending_app_availability_requests_.emplace(
      request_id,
      std::make_unique<GetAppAvailabilityRequest>(app_id, std::move(callback)));

  // XXX: need a timeout amount.
  VLOG(0) << "XXX: " << __func__ << ", app_id: " << app_id
          << ", request_id: " << request_id;
  cast_channel::CastMessage app_availability_request =
      cast_channel::CreateGetAppAvailabilityRequest(sender_id_, request_id,
                                                    app_id);
  SendCastMessage(*socket, app_availability_request);
}

void CastMessageHandler::OnError(const cast_channel::CastSocket& socket,
                                 cast_channel::ChannelError error_state) {
  // XXX: Consider optimizing by erroring out all pending requests for the
  // socket?
}

void CastMessageHandler::OnMessage(const cast_channel::CastSocket& socket,
                                   const cast_channel::CastMessage& message) {
  // XXX: Handle VC close messages.
  // (1) Check if it is a response
  // (2) Check if there is a corresponding request for that socket
  // (3) Process response accordingly
  // TODO(imcheng): Handle other messages.
  VLOG(0) << "XXX: " << __func__ << ": sourceid: " << message.source_id()
          << ", destid: " << message.destination_id()
          << ", namespace: " << message.namespace_()
          << ", msg: " << message.payload_utf8();
  if (!cast_channel::IsReceiverMessage(message) ||
      !cast_channel::IsPlatformSenderMessage(message))
    return;

  std::unique_ptr<base::Value> payload =
      cast_channel::GetDictionaryFromCastMessage(message);
  if (!payload)
    return;

  int request_id = 0;
  if (!cast_channel::GetRequestIdFromResponse(*payload, &request_id))
    return;

  auto it = pending_app_availability_requests_.find(request_id);
  if (it != pending_app_availability_requests_.end()) {
    cast_channel::GetAppAvailabilityResult result =
        cast_channel::GetAppAvailabilityResultFromResponse(*payload,
                                                           it->second->app_id);
    VLOG(0) << "XXX: " << __func__ << ", request_id: " << request_id
            << ", result: " << static_cast<int>(result);
    std::move(it->second->callback).Run(it->second->app_id, result);
    pending_app_availability_requests_.erase(it);
  } else
    VLOG(0) << "XXX: no corresponding request id: " << request_id;
}

void CastMessageHandler::SendCastMessage(
    const cast_channel::CastSocket& socket,
    const cast_channel::CastMessage& message) {
  // A virtual connection must be opened to the receiver before other messages
  // can be sent.
  VirtualConnection connection(socket.id(), message.source_id(),
                               message.destination_id());
  if (virtual_connections_.find(connection) == virtual_connections_.end()) {
    VLOG(0) << "XXX: opening vc, channel: " << connection.channel_id
            << ", source: " << connection.source_id
            << ", dest: " << connection.destination_id;
    cast_channel::CastMessage virtual_connection_request =
        cast_channel::CreateVirtualConnectionRequest(message.source_id(),
                                                     message.destination_id());
    // XXX: WeakPtr()
    // TODO(https://crbug.com/656607): Add proper annotation.
    socket.transport()->SendMessage(
        virtual_connection_request,
        base::Bind(&CastMessageHandler::OnMessageSent, base::Unretained(this)),
        NO_TRAFFIC_ANNOTATION_BUG_656607);
    virtual_connections_.insert(connection);
  }
  // XXX: WeakPtr()
  // TODO(https://crbug.com/656607): Add proper annotation.
  socket.transport()->SendMessage(
      message,
      base::Bind(&CastMessageHandler::OnMessageSent, base::Unretained(this)),
      NO_TRAFFIC_ANNOTATION_BUG_656607);
}

void CastMessageHandler::OnMessageSent(int result) {
  // XXX: do something with result
  VLOG(0) << "XXX: " << __func__ << ": " << result;
}

}  // namespace media_router
