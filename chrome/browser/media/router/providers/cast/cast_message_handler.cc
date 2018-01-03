// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_message_handler.h"

#include "components/cast_channel/cast_socket_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace media_router {

GetAppAvailabilityRequest::GetAppAvailabilityRequest(
    const std::string& app_id,
    GetAppAvailabilityCallback callback)
    : app_id(app_id), callback(std::move(callback)) {}

GetAppAvailabilityRequest::~GetAppAvailabilityRequest() = default;

CastMessageHandler::CastMessageHandler(
    cast_channel::CastSocketService* socket_service)
    : socket_service_(socket_service) {
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
  // XXX: WeakPtr()
  // TODO(https://crbug.com/656607): Add proper annotation.
  socket->transport()->SendMessage(
      cast_channel::CreateGetAppAvailabilityRequest(request_id, app_id),
      base::BindOnce(&CastMessageHandler::OnMessageSent,
                     base::Unretained(this)),
      NO_TRAFFIC_ANNOTATION_BUG_656607);
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
  if (message.namespace_() != cast_channel::kReceiverNamespace ||
      message.destination_id() != cast_channel::kPlatformSenderId)
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
    std::move(it->second->callback).Run(result);
    pending_app_availability_requests_.erase(it);
  }
}

void CastMessageHandler::OnMessageSent(int result) {
  // XXX: do something with result
}

}  // namespace media_router
