// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/network_id.h"

#include <tuple>

#include "base/strings/string_number_conversions.h"

namespace {

const char kValueSeparator = ',';

// Parses |connection_type_string| as a NetworkChangeNotifier::ConnectionType.
// |connection_type_string| must contain the
// NetworkChangeNotifier::ConnectionType enum as an integer.
net::NetworkChangeNotifier::ConnectionType ConvertStringToConnectionType(
    const std::string& connection_type_string) {
  int connection_type_int =
      static_cast<int>(net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  bool connection_type_available =
      base::StringToInt(connection_type_string, &connection_type_int);

  if (!connection_type_available || connection_type_int < 0 ||
      connection_type_int >
          static_cast<int>(net::NetworkChangeNotifier::CONNECTION_LAST)) {
    DCHECK(false);
    return net::NetworkChangeNotifier::CONNECTION_UNKNOWN;
  }
  return static_cast<net::NetworkChangeNotifier::ConnectionType>(
      connection_type_int);
}

int ConvertStringToSignalStrength(const std::string& signal_strength_string) {
  int signal_strength_int = 0;
  bool available =
      base::StringToInt(signal_strength_string, &signal_strength_int);

  return available ? signal_strength_int : 0;
}

}  // namespace

namespace net {
namespace nqe {
namespace internal {

// static
NetworkID NetworkID::FromString(const std::string& network_id) {
  size_t first_separator_index = network_id.find(kValueSeparator);
  DCHECK_NE(std::string::npos, first_separator_index);
  if (first_separator_index == std::string::npos) {
    return NetworkID(NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string(),
                     0);
  }

  size_t second_separator_index =
      network_id.find(kValueSeparator, first_separator_index + 1);

  if (second_separator_index == std::string::npos) {
    return NetworkID(ConvertStringToConnectionType(
                         network_id.substr(first_separator_index + 1)),
                     network_id.substr(0, first_separator_index), 0);
  }

  return NetworkID(ConvertStringToConnectionType(network_id.substr(
                       first_separator_index + 1,
                       second_separator_index - first_separator_index - 1)),
                   network_id.substr(0, first_separator_index),
                   ConvertStringToSignalStrength(
                       network_id.substr(second_separator_index + 1)));
}

NetworkID::NetworkID(NetworkChangeNotifier::ConnectionType type,
                     const std::string& id,
                     int signal_strength)
    : type(type), id(id), signal_strength(signal_strength) {}

NetworkID::NetworkID(const NetworkID& other)
    : type(other.type), id(other.id), signal_strength(other.signal_strength) {}

NetworkID::~NetworkID() {}

bool NetworkID::operator==(const NetworkID& other) const {
  return type == other.type && id == other.id &&
         signal_strength == other.signal_strength;
}

bool NetworkID::operator!=(const NetworkID& other) const {
  return !operator==(other);
}

NetworkID& NetworkID::operator=(const NetworkID& other) {
  type = other.type;
  id = other.id;
  signal_strength = other.signal_strength;
  return *this;
}

// Overloaded to support ordered collections.
bool NetworkID::operator<(const NetworkID& other) const {
  return std::tie(type, id, signal_strength) <
         std::tie(other.type, other.id, other.signal_strength);
}

std::string NetworkID::ToString() const {
  return id + kValueSeparator + base::IntToString(static_cast<int>(type)) +
         kValueSeparator + base::IntToString(signal_strength);
}

}  // namespace internal
}  // namespace nqe
}  // namespace net
