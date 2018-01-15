// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_message_handler.h"

#include <tuple>

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/default_tick_clock.h"
#include "components/cast_channel/cast_socket_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace media_router {

GetAppAvailabilityRequest::GetAppAvailabilityRequest(
    const std::string& app_id,
    GetAppAvailabilityCallback callback,
    base::TickClock* clock)
    : app_id(app_id), callback(std::move(callback)), timeout_timer(clock) {}

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
      new CastMessageHandler(cast_channel::CastSocketService::GetInstance(),
                             base::DefaultTickClock::GetInstance());
  return instance;
}

CastMessageHandler::CastMessageHandler(
    cast_channel::CastSocketService* socket_service,
    base::TickClock* clock)
    : sender_id_(base::StringPrintf("sender-%d", base::RandInt(0, 1000000))),
      socket_service_(socket_service),
      clock_(clock),
      weak_ptr_factory_(this) {
  socket_service_->AddObserver(this);
}

CastMessageHandler::~CastMessageHandler() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  socket_service_->RemoveObserver(this);
}

void CastMessageHandler::RequestAppAvailability(
    cast_channel::CastSocket* socket,
    const std::string& app_id,
    GetAppAvailabilityCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int request_id = NextRequestId();

  // XXX: need a timeout amount.
  DVLOG(2) << __func__ << ", socket_id: " << socket->id()
           << ", app_id: " << app_id << ", request_id: " << request_id;
  cast_channel::CastMessage message =
      cast_channel::CreateGetAppAvailabilityRequest(sender_id_, request_id,
                                                    app_id);

  auto request = std::make_unique<GetAppAvailabilityRequest>(
      app_id, std::move(callback), clock_);
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
      .Run(it->second->app_id,
           cast_channel::GetAppAvailabilityResult::kUnknown);
  pending_app_availability_requests_.erase(it);
}

void CastMessageHandler::OnError(const cast_channel::CastSocket& socket,
                                 cast_channel::ChannelError error_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Note that we can preemptively error out all pending requests here, but it
  // is unlikely to be helpful since the device will be removed on socket error
  // anyway.
}

void CastMessageHandler::OnMessage(const cast_channel::CastSocket& socket,
                                   const cast_channel::CastMessage& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(imcheng): Handle other messages.
  DVLOG(2) << __func__ << ", socket_id: " << socket.id()
           << ", message: " << CastMessageToString(message);
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
    std::move(it->second->callback).Run(it->second->app_id, result);
    pending_app_availability_requests_.erase(it);
  }
}

void CastMessageHandler::SendCastMessage(
    cast_channel::CastSocket* socket,
    const cast_channel::CastMessage& message) {
  // A virtual connection must be opened to the receiver before other messages
  // can be sent.
  VirtualConnection connection(socket->id(), message.source_id(),
                               message.destination_id());
  if (virtual_connections_.find(connection) == virtual_connections_.end()) {
    DVLOG(1) << "Creating VC for channel: " << connection.channel_id
             << ", source: " << connection.source_id
             << ", dest: " << connection.destination_id;
    cast_channel::CastMessage virtual_connection_request =
        cast_channel::CreateVirtualConnectionRequest(message.source_id(),
                                                     message.destination_id());
    // TODO(https://crbug.com/656607): Add proper annotation.
    socket->transport()->SendMessage(
        virtual_connection_request,
        base::Bind(&CastMessageHandler::OnMessageSent,
                   weak_ptr_factory_.GetWeakPtr()),
        NO_TRAFFIC_ANNOTATION_BUG_656607);

    // We assume the virtual connection request will succeed; otherwise this
    // will eventually self-correct.
    virtual_connections_.insert(connection);
  }
  // TODO(https://crbug.com/656607): Add proper annotation.
  socket->transport()->SendMessage(
      message,
      base::Bind(&CastMessageHandler::OnMessageSent,
                 weak_ptr_factory_.GetWeakPtr()),
      NO_TRAFFIC_ANNOTATION_BUG_656607);
}

void CastMessageHandler::OnMessageSent(int result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG_IF(2, result < 0) << "SendMessage failed with code: " << result;
}

}  // namespace media_router
