// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_MOBILE_PORT_ALLOCATOR_FACTORY_H_
#define REMOTING_PROTOCOL_MOBILE_PORT_ALLOCATOR_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "remoting/protocol/port_allocator_factory.h"
#include "third_party/webrtc/rtc_base/network.h"

namespace remoting {
namespace protocol {

// A factory that creates port allocator optimized for mobile devices, which
// removes all cellular networks if any WiFi network presents.
class MobilePortAllocatorFactory : public PortAllocatorFactory {
 public:
  MobilePortAllocatorFactory();
  ~MobilePortAllocatorFactory() override;

  // PortAllocatorFactory interface.
  std::unique_ptr<cricket::PortAllocator> CreatePortAllocator(
      scoped_refptr<TransportContext> transport_context) override;

  // This exposes the internal FilterCellularNetworks function for unit test.
  static void FilterCellularNetworksForTest(
      rtc::NetworkManager::NetworkList* networks);

 private:
  DISALLOW_COPY_AND_ASSIGN(MobilePortAllocatorFactory);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_MOBILE_PORT_ALLOCATOR_FACTORY_H_
