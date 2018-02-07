// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_
#define MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_

#include "base/callback.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/interfaces/network_service.mojom.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"

namespace media {

class UdpTransportClientImpl final : public cast::PacketTransport,
                                     public network::mojom::UDPSocketReceiver {
 public:
  explicit UdpTransportClientImpl(const net::IPEndPoint& remote_endpoint,
                                  network::mojom::NetworkContextPtr context);

  ~UdpTransportClientImpl() override;

  // cast::PacketTransport implementations.
  bool SendPacket(cast::PacketRef packet, const base::Closure& cb) override;
  int64_t GetBytesSent() override;

  void StartReceiving(
      const cast::PacketReceiverCallbackWithStatus& packet_receiver) override;

  void StopReceiving() override;

  // network::mojom::UDPSocketReceiver implementation.
  void OnReceived(int32_t result,
                  const base::Optional<net::IPEndPoint>& src_addr,
                  base::Optional<base::span<const uint8_t>> data) override;

 private:
  void OnSent(int result);
  void OnSocketOpen(int result);

  cast::PacketReceiverCallbackWithStatus callback_;
  const net::IPEndPoint remote_endpoint_;
  network::mojom::NetworkContextPtr network_context_;
  network::mojom::UDPSocketPtr udp_socket_;
  mojo::Binding<network::mojom::UDPSocketReceiver> binding_;
  base::Closure network_clear_cb_;
  int64_t bytes_sent_ = 0;
  bool sending_allowed_ = false;
  base::WeakPtrFactory<UdpTransportClientImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UdpTransportClientImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_
