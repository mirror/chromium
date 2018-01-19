// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/mobile_port_allocator_factory.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "remoting/protocol/chromium_socket_factory.h"
#include "remoting/protocol/mobile_network_manager.h"
#include "remoting/protocol/port_allocator.h"
#include "remoting/protocol/transport_context.h"
#include "third_party/webrtc/rtc_base/ipaddress.h"
#include "third_party/webrtc/rtc_base/network_constants.h"

namespace remoting {
namespace protocol {

MobilePortAllocatorFactory::MobilePortAllocatorFactory() = default;
MobilePortAllocatorFactory::~MobilePortAllocatorFactory() = default;

std::unique_ptr<cricket::PortAllocator>
MobilePortAllocatorFactory::CreatePortAllocator(
    scoped_refptr<TransportContext> transport_context) {
  return base::MakeUnique<PortAllocator>(
      base::WrapUnique(new MobileNetworkManager()),
      base::WrapUnique(new ChromiumPacketSocketFactory()), transport_context);
}

}  // namespace protocol
}  // namespace remoting
