// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_NET_UDP_TRANSPORT_HOST_IMPL_H_
#define MEDIA_CAST_NET_UDP_TRANSPORT_HOST_IMPL_H_

#include "media/cast/net/cast_transport_defines.h"
#include "media/cast/net/udp_transport_interface.h"

namespace media {
namespace cast {

class UdpPacketPipeReader;
class UdpTransport;
class UdpTransportClient;

// This class wraps a UdpTransport that is used to send/receive UDP packets over
// network.
// TODO(xjz): Implements the mojo interface.
class UdpTransportHostImpl final : public UdpTransportHost {
 public:
  explicit UdpTransportHostImpl(std::unique_ptr<UdpTransport> transport);

  ~UdpTransportHostImpl() override;

  // UdpTransportHost implementation.
  void StartReceiving(UdpTransportClient* client) override;
  void StopReceiving() override;
  void StartSending(mojo::ScopedDataPipeConsumerHandle packet_pipe) override;

 private:
  friend class UdpTransportHostImplTest;

  // Called when a packet is received.
  bool OnPacketReceived(std::unique_ptr<Packet> packet);

  // Requests to read a packet from the mojo data pipe. The reading can be sync
  // or async depending on whether the data is available.
  void ReadNextPacket();

  // Sends the packet over network through |udp_transport_|.
  void SendPacket(std::unique_ptr<Packet> packet);

  std::unique_ptr<UdpTransport> udp_transport_;

  // Not own by this class. Set by StartReceiving() call, and is cleared when
  // StopReceiving() is called.
  UdpTransportClient* client_ = nullptr;

  // Created when StartSending() is called.
  std::unique_ptr<UdpPacketPipeReader> reader_;

  DISALLOW_COPY_AND_ASSIGN(UdpTransportHostImpl);
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_NET_UDP_TRANSPORT_HOST_IMPL_H_
