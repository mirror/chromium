// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/network_id.h"

#include <tuple>

#include "base/base64.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "net/nqe/proto/network_id_proto.pb.h"

namespace {

const char kValueSeparator = ',';

// Parses |connection_type_string| as a NetworkChangeNotifier::ConnectionType.
// |connection_type_string| must contain the
// NetworkChangeNotifier::ConnectionType enum as an integer.
net::NetworkChangeNotifier::ConnectionType
ConvertStringToConnectionTypeDeprecated(
    const std::string& connection_type_string) {
  int connection_type_int =
      static_cast<int>(net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  bool connection_type_available =
      base::StringToInt(connection_type_string, &connection_type_int);

  if (!connection_type_available || connection_type_int < 0 ||
      connection_type_int >
          static_cast<int>(net::NetworkChangeNotifier::CONNECTION_LAST)) {
    return net::NetworkChangeNotifier::CONNECTION_UNKNOWN;
  }
  return static_cast<net::NetworkChangeNotifier::ConnectionType>(
      connection_type_int);
}

}  // namespace

namespace net {
namespace nqe {
namespace internal {

base::Optional<nqe::internal::NetworkID> FromStringDeprecated(
    const std::string& network_id) {
  size_t separator_index = network_id.find(kValueSeparator);
  if (separator_index == std::string::npos) {
    return base::nullopt;
  }

  NetworkChangeNotifier::ConnectionType connection_type =
      ConvertStringToConnectionTypeDeprecated(
          network_id.substr(separator_index + 1));
  if (connection_type == NetworkChangeNotifier::CONNECTION_UNKNOWN)
    return base::nullopt;

  std::string id = network_id.substr(0, separator_index);
  if (id.empty())
    return base::nullopt;
  return nqe::internal::NetworkID(connection_type, id);
}

// static
NetworkID NetworkID::FromString(const std::string& network_id) {
  // First try to parse using the deprecated (de)serialization algorithm.
  base::Optional<NetworkID> network_id_deprecated =
      FromStringDeprecated(network_id);
  if (network_id_deprecated)
    return network_id_deprecated.value();

  std::string base64_decoded;
  if (!base::Base64Decode(network_id, &base64_decoded))
    return NetworkID(NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string());

  NetworkIDProto network_id_proto;
  if (!network_id_proto.ParseFromString(base64_decoded))
    return NetworkID(NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string());

  return NetworkID(static_cast<NetworkChangeNotifier::ConnectionType>(
                       network_id_proto.connection_type()),
                   network_id_proto.id());
}

NetworkID::NetworkID(NetworkChangeNotifier::ConnectionType type,
                     const std::string& id)
    : type(type), id(id) {}

NetworkID::NetworkID(const NetworkID& other) : type(other.type), id(other.id) {}

NetworkID::~NetworkID() {}

bool NetworkID::operator==(const NetworkID& other) const {
  return type == other.type && id == other.id;
}

bool NetworkID::operator!=(const NetworkID& other) const {
  return !operator==(other);
}

NetworkID& NetworkID::operator=(const NetworkID& other) {
  type = other.type;
  id = other.id;
  return *this;
}

// Overloaded to support ordered collections.
bool NetworkID::operator<(const NetworkID& other) const {
  return std::tie(type, id) < std::tie(other.type, other.id);
}

std::string NetworkID::ToString() const {
  NetworkIDProto network_id_proto;
  network_id_proto.set_connection_type(static_cast<int>(type));
  network_id_proto.set_id(id);

  std::string serialized_network_id;
  if (!network_id_proto.SerializeToString(&serialized_network_id))
    return "";

  std::string base64_encoded;
  base::Base64Encode(serialized_network_id, &base64_encoded);

  return base64_encoded;
}

}  // namespace internal
}  // namespace nqe
}  // namespace net
