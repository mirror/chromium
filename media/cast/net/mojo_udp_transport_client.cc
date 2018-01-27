// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/net/mojo_udp_transport_client.h"

#include "base/callback.h"
#include "media/cast/net/udp_packet_pipe.h"

namespace media {
namespace cast {

namespace {
constexpr int kDataPipeCapacity = 800 * 1024;
}  // namespace

MojoUdpTransportClient::MojoUdpTransportClient(mojom::UdpTransportPtr host)
    : binding_(this), weak_factory_(this) {
  transport_host_ = std::move(host);
}

MojoUdpTransportClient::~MojoUdpTransportClient() {}

bool MojoUdpTransportClient::SendPacket(cast::PacketRef packet,
                                        const base::RepeatingClosure& cb) {
  // SendPacket() is only allowed to be called when there is no other packet
  // pending.
  DCHECK(done_cb_.is_null());
  DCHECK(!is_pending_);
  DCHECK(!cb.is_null());

  bytes_sent_ += packet->data.size();
  done_cb_ = cb;

  if (!writer_) {
    std::unique_ptr<mojo::DataPipe> data_pipe =
        base::MakeUnique<mojo::DataPipe>(kDataPipeCapacity);
    writer_.reset(
        new UdpPacketPipeWriter(std::move(data_pipe->producer_handle)));
    transport_host_->StartSending(std::move(data_pipe->consumer_handle));
  }

  writer_->Write(packet,
                 base::BindOnce(&MojoUdpTransportClient::CompleteWritingPacket,
                                base::Unretained(this)));

  is_pending_ = !done_cb_.is_null();
  return !is_pending_;
}

void MojoUdpTransportClient::CompleteWritingPacket() {
  DCHECK(!done_cb_.is_null());
  base::RepeatingClosure cb = done_cb_;
  done_cb_ = base::RepeatingClosure();
  if (is_pending_) {
    is_pending_ = false;
    cb.Run();
  }
}

int64_t MojoUdpTransportClient::GetBytesSent() {
  return bytes_sent_;
}

void MojoUdpTransportClient::StartReceiving(
    const cast::PacketReceiverCallbackWithStatus& packet_receiver) {
  DCHECK(!packet_receiver.is_null());
  callback_ = packet_receiver;
  mojom::UdpTransportReceiverPtr this_client;
  binding_.Bind(mojo::MakeRequest(&this_client));
  transport_host_->StartReceiving(std::move(this_client));
}

void MojoUdpTransportClient::StopReceiving() {
  callback_ = cast::PacketReceiverCallbackWithStatus();
  transport_host_->StopReceiving();
}

void MojoUdpTransportClient::OnPacketReceived(
    const std::vector<uint8_t>& packet) {
  if (!callback_.is_null()) {
    std::unique_ptr<cast::Packet> received_packet(new cast::Packet());
    received_packet->swap(const_cast<cast::Packet&>(packet));
    callback_.Run(std::move(received_packet));
  }
}

}  // namespace cast
}  // namespace media
