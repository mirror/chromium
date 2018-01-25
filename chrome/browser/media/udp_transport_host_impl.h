// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_UDP_TRANSPORT_HOST_IMPL_H_
#define CHROME_BROWSER_MEDIA_UDP_TRANSPORT_HOST_IMPL_H_

#include "base/callback.h"
#include "media/cast/net/cast_transport_defines.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"

namespace media {

namespace cast {
class UdpTransport;
}  // namespace cast

class UdpTransportHostImpl final : public mojom::UdpTransportHost {
 public:
  explicit UdpTransportHostImpl(std::unique_ptr<cast::UdpTransport> transport);
  ~UdpTransportHostImpl() override;

  static void Create(
      std::unique_ptr<cast::UdpTransport> transport,
      base::OnceCallback<void(mojom::UdpTransportHostPtr)> callback);

  // mojom::UdpTransportHost implementations.
  void StartReceiving(mojom::UdpTransportClientPtr client) override;
  void StopReceiving() override;
  void StartSending(mojo::ScopedDataPipeConsumerHandle packet_pipe) override;

  void SendPacket(cast::PacketRef packet);
  void OnPacketSent();
  bool OnPacketReceived(std::unique_ptr<cast::Packet> packet);

 private:
  class MojoDataReader;
  std::unique_ptr<cast::UdpTransport> udp_transport_;
  mojom::UdpTransportClientPtr client_;
  bool send_pending_ = false;
  std::unique_ptr<MojoDataReader> reader_;

  DISALLOW_COPY_AND_ASSIGN(UdpTransportHostImpl);
};

}  // namespace media

#endif  // CHROME_BROWSER_MEDIA_UDP_TRANSPORT_HOST_IMPL_H_
