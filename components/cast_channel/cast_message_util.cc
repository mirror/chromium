// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_message_util.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "components/cast_channel/cast_auth_util.h"
#include "components/cast_channel/proto/cast_channel.pb.h"

using base::Value;

namespace {
// Text payload keys.
constexpr char kTypeNodeId[] = "type";
constexpr char kRequestIdNodeId[] = "requestId";
}  // namespace

namespace cast_channel {

bool IsCastMessageValid(const CastMessage& message_proto) {
  if (message_proto.namespace_().empty() || message_proto.source_id().empty() ||
      message_proto.destination_id().empty()) {
    return false;
  }
  return (message_proto.payload_type() == CastMessage_PayloadType_STRING &&
          message_proto.has_payload_utf8()) ||
         (message_proto.payload_type() == CastMessage_PayloadType_BINARY &&
          message_proto.has_payload_binary());
}

std::unique_ptr<Value> GetDictionaryFromCastMessage(
    const CastMessage& message) {
  if (!message.has_payload_utf8())
    return nullptr;

  std::unique_ptr<Value> parsed_payload(
      base::JSONReader::Read(message.payload_utf8()));
  if (!parsed_payload || parsed_payload->type() != Value::Type::DICTIONARY)
    return nullptr;

  return parsed_payload;
}

std::string CastMessageToString(const CastMessage& message_proto) {
  std::string out("{");
  out += "namespace = " + message_proto.namespace_();
  out += ", sourceId = " + message_proto.source_id();
  out += ", destId = " + message_proto.destination_id();
  out += ", type = " + base::IntToString(message_proto.payload_type());
  out += ", str = \"" + message_proto.payload_utf8() + "\"}";
  return out;
}

std::string AuthMessageToString(const DeviceAuthMessage& message) {
  std::string out("{");
  if (message.has_challenge()) {
    out += "challenge: {}, ";
  }
  if (message.has_response()) {
    out += "response: {signature: (";
    out += base::NumberToString(message.response().signature().length());
    out += " bytes), certificate: (";
    out += base::NumberToString(
        message.response().client_auth_certificate().length());
    out += " bytes)}";
  }
  if (message.has_error()) {
    out += ", error: {";
    out += base::IntToString(message.error().error_type());
    out += "}";
  }
  out += "}";
  return out;
}

void CreateAuthChallengeMessage(CastMessage* message_proto,
                                const AuthContext& auth_context) {
  CHECK(message_proto);
  DeviceAuthMessage auth_message;

  AuthChallenge* challenge = auth_message.mutable_challenge();
  DCHECK(challenge);
  challenge->set_sender_nonce(auth_context.nonce());
  challenge->set_hash_algorithm(SHA256);

  std::string auth_message_string;
  auth_message.SerializeToString(&auth_message_string);

  message_proto->set_protocol_version(CastMessage_ProtocolVersion_CASTV2_1_0);
  message_proto->set_source_id(kPlatformSenderId);
  message_proto->set_destination_id(kPlatformReceiverId);
  message_proto->set_namespace_(kAuthNamespace);
  message_proto->set_payload_type(CastMessage_PayloadType_BINARY);
  message_proto->set_payload_binary(auth_message_string);
}

bool IsAuthMessage(const CastMessage& message) {
  return message.namespace_() == kAuthNamespace;
}

bool GetRequestIdFromResponse(const Value& payload, int* request_id) {
  DCHECK(request_id);
  DCHECK(payload.type() == Value::Type::DICTIONARY);

  const Value* request_id_value =
      payload.FindKeyOfType(kRequestIdNodeId, Value::Type::INTEGER);
  if (!request_id_value)
    return false;

  *request_id = request_id_value->GetInt();
  return true;
}

CastMessage CreateGetAppAvailabilityRequest(int request_id,
                                            const std::string& app_id) {
  CastMessage output;
  output.set_protocol_version(CastMessage::CASTV2_1_0);
  output.set_source_id(kPlatformSenderId);
  output.set_destination_id(kPlatformReceiverId);
  output.set_namespace_(kReceiverNamespace);
  output.set_payload_type(
      CastMessage::PayloadType::CastMessage_PayloadType_STRING);

  Value dict(Value::Type::DICTIONARY);
  dict.SetKey(kTypeNodeId, Value(kGetAppAvailabilityRequestType));
  Value app_id_value(Value::Type::LIST);
  app_id_value.GetList().push_back(Value(app_id));
  dict.SetKey("appId", std::move(app_id_value));
  dict.SetKey(kRequestIdNodeId, Value(request_id));
  CHECK(base::JSONWriter::Write(dict, output.mutable_payload_utf8()));
  return output;
}

GetAppAvailabilityResult GetAppAvailabilityResultFromResponse(
    const Value& payload,
    const std::string& app_id) {
  DCHECK(payload.type() == Value::Type::DICTIONARY);
  const Value* availability_value =
      payload.FindPathOfType({"availability", app_id}, Value::Type::STRING);
  if (!availability_value)
    return GetAppAvailabilityResult::kUnknown;

  if (availability_value->GetString() == "APP_AVAILABLE")
    return GetAppAvailabilityResult::kAvailable;
  if (availability_value->GetString() == "APP_UNAVAILABLE")
    return GetAppAvailabilityResult::kUnavailable;

  return GetAppAvailabilityResult::kUnknown;
}

}  // namespace cast_channel
