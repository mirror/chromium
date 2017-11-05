// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/cast/cast_requests.h"

#include "components/cast_channel/cast_socket_service.h"

namespace media_router {

namespace {

// Creats a DictionaryValue that represents a request to get availability status
// for the app given by |app_id|. Always returns non-null.
std::unique_ptr<base::DictionaryValue> CreateGetAppAvailabilityRequest(
    const CastAppId& app_id) {
  auto value = std::make_unique<base::DictionaryValue>();
  value->SetPath({"type"}, base::Value("GET_APP_AVAILABILITY"));
  base::Value app_id_value(base::Value::Type::LIST);
  app_id_value.GetList().push_back(base::Value(app_id));
  value->SetPath({"appId"}, std::move(app_id_value));
  return value;
}

}  // namespace

CastMessageSender::CastMessageSender(
    cast_channel::CastSocketService* socket_service)
    : socket_service_(socket_service) {
  socket_service_->AddObserver(this);
}
CastMessageSender::~CastMessageSender() {
  socket_service_->RemoveObserver(this);
}

void CastMessageSender::RequestAppAvailability(
    const cast_channel::CastSocket& socket,
    const CastAppId& app_id,
    GetAppAvailabilityCallback callback) {
  // XXX: do we need to create VC before sending get_app_availability request?
  // XXX: need a timeout amount.
  auto request = CreateGetAppAvailabilityRequest(app_id);
  // XXX: Maybe make CreateGetAppAvailabilityRequest a private method that also
  // sets requestId?
  int request_id = NextRequestId();
  request->SetPath({"requestId"}, base::Value(request_id));
  // XXX: need to track response by request id.
  std::move(callback).Run(GetAppAvailabilityResult::kUnknown);
}

void CastMessageSender::OnError(const cast_channel::CastSocket& socket,
                                cast_channel::ChannelError error_state) {
  // XXX: Consider optimizing by erroring out all pending requests for the
  // socket?
}

void CastMessageSender::OnMessage(const cast_channel::CastSocket& socket,
                                  const cast_channel::CastMessage& message) {
  // (1) Check if it is a response
  // (2) Check if there is a corresponding request for that socket
  // (3) Process response accordingly
}

}  // namespace media_router
