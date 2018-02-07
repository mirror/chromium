// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/udp_transport_client_impl.h"

#include "net/base/address_family.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace media {

namespace {

#define TRAFFIC_ANNOTATION                      \
  net::DefineNetworkTrafficAnnotation("mirror", \
                                      "Traffic annotation for mirror service")

}  // namespace

UdpTransportClientImpl::UdpTransportClientImpl(
    const net::IPEndPoint& remote_endpoint,
    network::mojom::NetworkContextPtr context)
    : remote_endpoint_(remote_endpoint),
      network_context_(std::move(context)),
      binding_(this),
      weak_factory_(this) {}

UdpTransportClientImpl::~UdpTransportClientImpl() {}

bool UdpTransportClientImpl::SendPacket(cast::PacketRef packet,
                                        const base::Closure& cb) {
  bytes_sent_ += packet->data.size();

  if (!sending_allowed_) {
    network_clear_cb_ = cb;
    return false;
  }

  DCHECK(udp_socket_);
  udp_socket_->Send(packet->data,
                    net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION),
                    base::BindOnce(&UdpTransportClientImpl::OnSent,
                                   weak_factory_.GetWeakPtr()));
  return true;
}

void UdpTransportClientImpl::OnSent(int result) {
  if (result != net::OK) {
    sending_allowed_ = false;
    LOG(INFO) << __func__ << ": error=" << result;
    return;
  }

  sending_allowed_ = true;
  if (!network_clear_cb_.is_null())
    std::move(network_clear_cb_).Run();
}

int64_t UdpTransportClientImpl::GetBytesSent() {
  return bytes_sent_;
}

void UdpTransportClientImpl::StartReceiving(
    const cast::PacketReceiverCallbackWithStatus& packet_receiver) {
  DVLOG(1) << __func__;
  DCHECK(!packet_receiver.is_null());
  callback_ = packet_receiver;
  network::mojom::UDPSocketReceiverPtr udp_socket_receiver;
  binding_.Bind(mojo::MakeRequest(&udp_socket_receiver));

  network_context_->CreateUDPSocket(mojo::MakeRequest(&udp_socket_),
                                    std::move(udp_socket_receiver));
  udp_socket_->Open(net::ADDRESS_FAMILY_IPV4,
                    base::Bind(&UdpTransportClientImpl::OnSocketOpen,
                               weak_factory_.GetWeakPtr()));
}

void UdpTransportClientImpl::OnSocketOpen(int result) {
  if (result != net::OK) {
    sending_allowed_ = false;
    LOG(INFO) << "Socket open error.";
    return;
  }

  udp_socket_->Connect(remote_endpoint_,
                       base::BindOnce(
                           [](bool* sending_allowed, int result,
                              const base::Optional<net::IPEndPoint>& addr) {
                             LOG(INFO) << "socket open result=" << result;
                             *sending_allowed = (result == net::OK);
                           },
                           base::Unretained(&sending_allowed_)));
  udp_socket_->ReceiveMore(1);
}

void UdpTransportClientImpl::StopReceiving() {
  callback_ = cast::PacketReceiverCallbackWithStatus();
}

void UdpTransportClientImpl::OnReceived(
    int32_t result,
    const base::Optional<net::IPEndPoint>& src_addr,
    base::Optional<base::span<const uint8_t>> data) {
  udp_socket_->ReceiveMore(1);
  if (result != net::OK)
    return;
  DCHECK(data);

  std::unique_ptr<cast::Packet> packet(
      new cast::Packet(data.value().begin(), data.value().end()));
  DCHECK(!callback_.is_null());
  callback_.Run(std::move(packet));
}

}  // namespace media
