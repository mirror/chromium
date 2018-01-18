// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_MOBILE_NETWORK_MANAGER_H_
#define REMOTING_PROTOCOL_MOBILE_NETWORK_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "third_party/webrtc/rtc_base/network.h"

namespace remoting {
namespace protocol {

// A network manager that removes all cellular networks if any WiFi network
// presents.
class MobileNetworkManager : public rtc::BasicNetworkManager {
 public:
  MobileNetworkManager();
  ~MobileNetworkManager() override;

  // NetworkManager overrides.
  void GetNetworks(NetworkList* networks) const override;
  void GetAnyAddressNetworks(NetworkList* networks) override;

 private:
  // Removes all cellular networks if any WiFi network presents.
  static void FilterCellularNetworks(NetworkList* networks);

  friend class MobileNetworkManagerTest;

  DISALLOW_COPY_AND_ASSIGN(MobileNetworkManager);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_MOBILE_NETWORK_MANAGER_H_
