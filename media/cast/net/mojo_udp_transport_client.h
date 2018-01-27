// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_NET_MOJO_UDP_TRANSPORT_CLIENT_H_
#define MEDIA_CAST_NET_MOJO_UDP_TRANSPORT_CLIENT_H_

#include "base/callback.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/mojo/interfaces/udp_transport.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
namespace cast {

class UdpPacketPipeWriter;

// This class is a UDP packet transport client that connects to a transport host
// which implements mojom::UdpTransport. When sending packets, the packets are
// sent first to the transport host through a mojo data pipe, and then are sent
// out over the network by the transport host. When the transport host receives
// any packet, it will deliever the packet to this client.
class MojoUdpTransportClient final : public PacketTransport,
                                     public mojom::UdpTransportReceiver {
 public:
  explicit MojoUdpTransportClient(mojom::UdpTransportPtr host);

  ~MojoUdpTransportClient() override;

  // cast::PacketTransport implementations.
  // TODO(xjz): Change this interface to use base::OnceClosure instead.
  bool SendPacket(cast::PacketRef packet,
                  const base::RepeatingClosure& cb) override;
  int64_t GetBytesSent() override;
  void StartReceiving(
      const cast::PacketReceiverCallbackWithStatus& packet_receiver) override;
  void StopReceiving() override;

  // mojom::UdpTransportReceiver implementation.
  void OnPacketReceived(const std::vector<uint8_t>& packet) override;

 private:
  // Called by |writer_| when finishes writing a packet. This callback might be
  // called either sync or async when requests to write a packet.
  void CompleteWritingPacket();

  // Total numbe of bytes written to the data pipe.
  int64_t bytes_sent_ = 0;

  // The callback to deliver the received packets to the packet parser. Set
  // when StartReceiving() is called.
  cast::PacketReceiverCallbackWithStatus callback_;

  // The transport host that reads packet from the data pipe and sends them out
  // over network.
  mojom::UdpTransportPtr transport_host_;

  mojo::Binding<mojom::UdpTransportReceiver> binding_;

  // Set by SendPacket() when writing the packet to the data pipe is blocked.
  // This will run after the writing is completed. Once set, SendPacket() can
  // only be called again after this runs.
  // TODO(xjz): Changed to base::OnceClosure after the SendPacket() interface
  // is changed.
  base::RepeatingClosure done_cb_;

  // A mojo data pipe writer used to pass the UDP packets to the transport host.
  std::unique_ptr<UdpPacketPipeWriter> writer_;

  // Indicates that the writing to the data pipe is blocked.
  bool is_pending_ = false;

  base::WeakPtrFactory<MojoUdpTransportClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoUdpTransportClient);
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_
