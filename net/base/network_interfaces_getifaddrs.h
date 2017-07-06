// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NETWORK_INTERFACES_GETIFADDRS_H_
#define NET_BASE_NETWORK_INTERFACES_GETIFADDRS_H_

#include "net/base/network_interfaces.h"

#include <string>

struct ifaddrs;

namespace net {
namespace internal {

class NET_EXPORT IPAttributesGetter {
 public:
  IPAttributesGetter() {}
  virtual ~IPAttributesGetter() {}
  virtual bool IsInitialized() const = 0;

  // Returns false if the interface must be skipped. Otherwise sets |attributes|
  // and returns true.
  virtual bool GetAddressAttributes(const ifaddrs* if_addr,
                                    int* attributes) = 0;

  // Returns interface type for the given interface.
  virtual NetworkChangeNotifier::ConnectionType GetNetworkInterfaceType(
      const ifaddrs* if_addr) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(IPAttributesGetter);
};

NET_EXPORT bool IfaddrsToNetworkInterfaceList(
    int policy,
    const ifaddrs* interfaces,
    IPAttributesGetter* ip_attributes_getter,
    NetworkInterfaceList* networks);

}  // namespace internal
}  // namespace net

#endif  // NET_BASE_NETWORK_INTERFACES_GETIFADDRS_H_
