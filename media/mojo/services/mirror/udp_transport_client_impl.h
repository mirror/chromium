// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_
#define MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_

#include "base/callback.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class UdpTransportClientImpl final : public cast::PacketTransport,
                                     public mojom::UdpTransportClient {
 public:
  explicit UdpTransportClientImpl(mojom::UdpTransportHostPtr host);

  ~UdpTransportClientImpl() override;

  // cast::PacketTransport implementations.
  bool SendPacket(cast::PacketRef packet, const base::Closure& cb) override;
  int64_t GetBytesSent() override;

  void StartReceiving(
      const cast::PacketReceiverCallbackWithStatus& packet_receiver) override;

  void StopReceiving() override;

  // mojom::UdpTransportClient implementation.
  void OnPacketReceived(const std::vector<uint8_t>& packet) override;

  void OnNetworkBlocked(bool blocked) override;

 private:
  class MojoDataPipeWriter;
  mojom::UdpTransportHost* GetUdpHost();

  int64_t bytes_sent_ = 0;
  bool send_pending_ = false;
  cast::PacketReceiverCallbackWithStatus callback_;
  mojom::UdpTransportHostPtrInfo udp_host_info_;
  mojom::UdpTransportHostPtr udp_host_;
  mojo::Binding<mojom::UdpTransportClient> binding_;
  base::Closure network_clear_cb_;
  std::unique_ptr<MojoDataPipeWriter> writer_;
  base::WeakPtrFactory<UdpTransportClientImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UdpTransportClientImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_UDP_TRANSPORT_CLIENT_IMPL_H_
