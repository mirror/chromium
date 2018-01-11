// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mirroring/udp_transport_host_impl.h"

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "media/cast/net/udp_packet_pipe.h"
#include "media/cast/net/udp_transport.h"

namespace media {

UdpTransportHostImpl::UdpTransportHostImpl(
    std::unique_ptr<cast::UdpTransport> transport)
    : udp_transport_(std::move(transport)) {
  DCHECK(udp_transport_);
}

UdpTransportHostImpl::~UdpTransportHostImpl() {}

void UdpTransportHostImpl::StartReceiving(UdpTransportClient* client) {
  DCHECK(client);
  client_ = std::move(client);
  udp_transport_->StartReceiving(base::BindRepeating(
      &UdpTransportHostImpl::OnPacketReceived, base::Unretained(this)));
}

void UdpTransportHostImpl::StopReceiving() {
  client_ = nullptr;
  udp_transport_->StopReceiving();
}

void UdpTransportHostImpl::StartSending(
    mojo::ScopedDataPipeConsumerHandle packet_pipe) {
  DCHECK(packet_pipe.is_valid());
  DCHECK(!reader_);

  reader_.reset(new cast::UdpPacketPipeReader(std::move(packet_pipe)));
  ReadNextPacket();
}

bool UdpTransportHostImpl::OnPacketReceived(
    std::unique_ptr<cast::Packet> packet) {
  DVLOG(3) << __func__;
  if (!client_)
    return true;
  client_->OnPacketReceived(*packet);
  return true;
}

void UdpTransportHostImpl::ReadNextPacket() {
  reader_->Read(base::BindOnce(&UdpTransportHostImpl::SendPacket,
                               base::Unretained(this)));
}

void UdpTransportHostImpl::SendPacket(std::unique_ptr<cast::Packet> packet) {
  DVLOG(3) << __func__;
  if (!udp_transport_->SendPacket(
          base::WrapRefCounted(new base::RefCountedData<cast::Packet>(*packet)),
          base::BindRepeating(&UdpTransportHostImpl::ReadNextPacket,
                              base::Unretained(this)))) {
    return;  // Waiting for the packet to be sent out.
  }
  // Force a post task to prevent the stack from growing too deep.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&UdpTransportHostImpl::ReadNextPacket,
                                base::Unretained(this)));
}

}  // namespace media
