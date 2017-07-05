// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/network_interfaces_mac.h"

#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <memory>
#include <set>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "net/base/escape.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/network_interfaces_posix.h"
#include "url/gurl.h"

namespace net {

namespace internal {

bool GetNetworkListImpl(NetworkInterfaceList* networks,
                        int policy,
                        const ifaddrs* interfaces) {
  // Enumerate the addresses assigned to network interfaces which are up.
  for (const ifaddrs* interface = interfaces; interface != NULL;
       interface = interface->ifa_next) {
    // Skip loopback interfaces, and ones which are down.
    if (!(IFF_RUNNING & interface->ifa_flags))
      continue;
    if (IFF_LOOPBACK & interface->ifa_flags)
      continue;
    // Skip interfaces with no address configured.
    struct sockaddr* addr = interface->ifa_addr;
    if (!addr)
      continue;

    // Skip unspecified addresses (i.e. made of zeroes) and loopback addresses
    // configured on non-loopback interfaces.
    if (IsLoopbackOrUnspecifiedAddress(addr))
      continue;

    const std::string& name = interface->ifa_name;
    // Filter out VMware interfaces, typically named vmnet1 and vmnet8.
    if (ShouldIgnoreInterface(name, policy)) {
      continue;
    }

    NetworkChangeNotifier::ConnectionType connection_type =
        NetworkChangeNotifier::CONNECTION_UNKNOWN;

    int ip_attributes = IP_ADDRESS_ATTRIBUTE_NONE;

    IPEndPoint address;

    int addr_size = 0;
    if (addr->sa_family == AF_INET6) {
      addr_size = sizeof(sockaddr_in6);
    } else if (addr->sa_family == AF_INET) {
      addr_size = sizeof(sockaddr_in);
    }

    if (address.FromSockAddr(addr, addr_size)) {
      uint8_t prefix_length = 0;
      if (interface->ifa_netmask) {
        // If not otherwise set, assume the same sa_family as ifa_addr.
        if (interface->ifa_netmask->sa_family == 0) {
          interface->ifa_netmask->sa_family = addr->sa_family;
        }
        IPEndPoint netmask;
        if (netmask.FromSockAddr(interface->ifa_netmask, addr_size)) {
          prefix_length = MaskPrefixLength(netmask.address());
        }
      }
      networks->push_back(NetworkInterface(
          name, name, if_nametoindex(name.c_str()), connection_type,
          address.address(), prefix_length, ip_attributes));
    }
  }

  return true;
}

}  // namespace internal

bool GetNetworkList(NetworkInterfaceList* networks, int policy) {
  if (!networks)
    return false;

  // getifaddrs() may require IO operations.
  base::ThreadRestrictions::AssertIOAllowed();

  ifaddrs* interfaces;
  if (getifaddrs(&interfaces) < 0) {
    PLOG(ERROR) << "getifaddrs";
    return false;
  }

  bool result = internal::GetNetworkListImpl(networks, policy, interfaces);

  freeifaddrs(interfaces);
  return result;
}

std::string GetWifiSSID() {
  // TODO(fuchsia): Implement this function.
  NOTIMPLEMENTED();
  return std::string();
}

}  // namespace net
