// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/cast/cast_requests.h"

#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/cast_channel/proto/cast_channel.pb.h"

using base::Value;

namespace media_router {

namespace {

constexpr char kReceiverZeroDestinationId[] = "receiver-0";

constexpr char kReceiverNamespace[] = "urn:x-cast:com.google.cast.receiver";

std::string ToJSONString(const Value& value) {
  // TODO(imcheng): check if |value| is string type.
  std::string value_string;
  JSONStringValueSerializer data_serializer(&value_string);
  bool success = data_serializer.Serialize(value);
  DCHECK(success);
  return value_string;
}

std::unique_ptr<Value> FromJSONString(const std::string& value_str) {
  JSONStringValueDeserializer deserializer(value_str, base::JSON_PARSE_RFC);
  int error_code = 0;
  std::string error_message;
  auto value = deserializer.Deserialize(&error_code, &error_message);
  DVLOG_IF(!value, 2) << "JSON deserialize failed with code: " << error_code
                      << ", " << error_message;
  return value;
}

void FillCastMessage(const std::string& source_id,
                     const std::string& destination_id,
                     const std::string& message_namespace,
                     const std::string& data,
                     cast_channel::CastMessage* message_proto) {
  message_proto->set_protocol_version(
      cast_channel::CastMessage_ProtocolVersion_CASTV2_1_0);
  message_proto->set_source_id(source_id);
  message_proto->set_destination_id(destination_id);
  message_proto->set_namespace_(message_namespace);
  message_proto->set_payload_type(
      ::cast_channel::CastMessage_PayloadType_STRING);
  message_proto->set_payload_utf8(data);
}

}  // namespace

GetAppAvailabilityRequest::GetAppAvailabilityRequest(
    const CastAppId& app_id,
    GetAppAvailabilityCallback callback)
    : app_id(app_id), callback(std::move(callback)) {}

GetAppAvailabilityRequest::~GetAppAvailabilityRequest() = default;

CastMessageSender::CastMessageSender(
    cast_channel::CastSocketService* socket_service)
    : socket_service_(socket_service) {
  socket_service_->AddObserver(this);
}
CastMessageSender::~CastMessageSender() {
  socket_service_->RemoveObserver(this);
}

void CastMessageSender::RequestAppAvailability(
    cast_channel::CastSocket* socket,
    const CastAppId& app_id,
    GetAppAvailabilityCallback callback) {
  int request_id = NextRequestId();
  auto request = CreateGetAppAvailabilityRequest(request_id, app_id);

  pending_app_availability_requests_.emplace(
      request_id,
      std::make_unique<GetAppAvailabilityRequest>(app_id, std::move(callback)));

  SendMessage(socket, ToJSONString(request), kReceiverNamespace,
              default_sender_id_, kReceiverZeroDestinationId);
}

void CastMessageSender::OnError(const cast_channel::CastSocket& socket,
                                cast_channel::ChannelError error_state) {
  // XXX: Consider optimizing by erroring out all pending requests for the
  // socket?
}

void CastMessageSender::OnMessage(const cast_channel::CastSocket& socket,
                                  const cast_channel::CastMessage& message) {
  // (0) Handle VC close messages.
  // (1) Check if it is a response
  // (2) Check if there is a corresponding request for that socket
  // (3) Process response accordingly
  if (message.namespace_() != kReceiverNamespace)
    return;

  if (!message.has_payload_utf8())
    return;

  if (message.destination_id() != default_sender_id_)
    return;

  auto value = FromJSONString(message.payload_utf8());
  if (!value || value->type() != Value::Type::DICTIONARY)
    return;

  const auto* request_id =
      value->FindKeyOfType("requestId", Value::Type::INTEGER);
  if (!request_id)
    return;

  auto it = pending_app_availability_requests_.find(request_id->GetInt());
  if (it == pending_app_availability_requests_.end())
    return;

  const auto* availability_value = value->FindPathOfType(
      {"availability", it->second->app_id}, Value::Type::STRING);
  if (!availability_value)
    return;

  const auto& availability = availability_value->GetString();
  GetAppAvailabilityResult result = GetAppAvailabilityResult::kUnknown;
  if (availability == "APP_AVAILABLE")
    result = GetAppAvailabilityResult::kAvailable;
  else if (availability == "APP_UNAVAILABLE")
    result = GetAppAvailabilityResult::kUnavailable;

  std::move(it->second->callback).Run(result);
  pending_app_availability_requests_.erase(it);
}

void CastMessageSender::SendMessage(cast_channel::CastSocket* socket,
                                    const std::string& source_id,
                                    const std::string& destination_id,
                                    const std::string& message_namespace,
                                    const std::string& data) {
  // XXX: do we need to create VC before sending get_app_availability request?
  // XXX: need a timeout amount.
  cast_channel::CastMessage message_proto;
  FillCastMessage(source_id, destination_id, message_namespace, data,
                  &message_proto);
  // XXX: AsWeakPtr()
  socket->transport()->SendMessage(
      message_proto,
      base::Bind(&CastMessageSender::OnMessageSent, base::Unretained(this)));
}

void CastMessageSender::OnMessageSent(int result) {
  // XXX: do something with result
}

Value CastMessageSender::CreateGetAppAvailabilityRequest(
    int request_id,
    const CastAppId& app_id) {
  Value request(Value::Type::DICTIONARY);
  request.SetKey("type", Value("GET_APP_AVAILABILITY"));
  Value app_id_value(Value::Type::LIST);
  app_id_value.GetList().push_back(Value(app_id));
  request.SetKey("appId", std::move(app_id_value));
  request.SetKey("requestId", Value(request_id));
  return request;
}

}  // namespace media_router
