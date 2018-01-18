// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/mobile_network_manager.h"

#include "base/logging.h"
#include "third_party/webrtc/rtc_base/ipaddress.h"
#include "third_party/webrtc/rtc_base/network_constants.h"

namespace {

bool IsConnectedWifiNetwork(const rtc::Network& network) {
  if (network.type() != rtc::AdapterType::ADAPTER_TYPE_WIFI) {
    return false;
  }

  // Link local IP is potentially from a disconnected interface. For example
  // iOS always has a network on en2 with IP 169.254.x.x even when WiFi is
  // disabled. In this case it's safer not to filter out cellular networks. Note
  // that the link local network itself won't be filtered out.
  rtc::IPAddress ip = network.GetBestIP();
  switch (ip.family()) {
    case AF_INET: {
      uint32_t ip_in_host_order = ip.v4AddressAsHostOrderInteger();
      bool is_link_local = (ip_in_host_order >> 16) == ((169 << 8) | 254);
      return !is_link_local;
    }
    case AF_INET6: {
      in6_addr addr = ip.ipv6_address();
      bool is_link_local = addr.s6_addr[0] == 0xFE && addr.s6_addr[1] == 0x80;
      return !is_link_local;
    }
    default:
      LOG(ERROR) << "Unknown IP family: " << ip.family();
      return false;
  }
}

}  // namespace

namespace remoting {
namespace protocol {

MobileNetworkManager::MobileNetworkManager() = default;

MobileNetworkManager::~MobileNetworkManager() = default;

void MobileNetworkManager::GetNetworks(NetworkList* networks) const {
  rtc::BasicNetworkManager::GetNetworks(networks);
  FilterCellularNetworks(networks);
}

void MobileNetworkManager::GetAnyAddressNetworks(NetworkList* networks) {
  rtc::BasicNetworkManager::GetAnyAddressNetworks(networks);
  FilterCellularNetworks(networks);
}

// static
void MobileNetworkManager::FilterCellularNetworks(NetworkList* networks) {
  bool has_wifi_network = false;
  for (rtc::Network* network : *networks) {
    if (IsConnectedWifiNetwork(*network)) {
      has_wifi_network = true;
      break;
    }
  }

  if (!has_wifi_network) {
    return;
  }

  for (auto it = networks->begin(); it != networks->end();) {
    rtc::Network* network = *it;
    if (network->type() == rtc::AdapterType::ADAPTER_TYPE_CELLULAR) {
      networks->erase(it);
      VLOG(1) << "Removed cellular network: " << network->name();
    } else {
      it++;
    }
  }
}

}  // namespace protocol
}  // namespace remoting
