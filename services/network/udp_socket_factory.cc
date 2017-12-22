// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/udp_socket_factory.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "net/base/net_errors.h"
#include "services/network/udp_socket.h"

namespace network {

UDPSocketFactory::UDPSocketFactory() {}

UDPSocketFactory::~UDPSocketFactory() {}

void UDPSocketFactory::AddRequest(mojom::UDPSocketFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void UDPSocketFactory::OpenAndBind(
    network::mojom::UDPSocketRequest request,
    mojom::UDPSocketOptionsPtr socket_options,
    network::mojom::UDPSocketReceiverPtr receiver,
    const net::IPEndPoint& local_addr,
    OpenAndBindCallback callback) {
  auto socket =
      std::make_unique<UDPSocket>(std::move(request), std::move(receiver));
  int result = socket->Open(local_addr.GetFamily());
  // Parse socket options.
  if (result == net::OK && socket_options) {
    if (socket_options->allow_address_reuse) {
      result = socket->SetAllowAddressReuse();
    }
    if (result == net::OK) {
      result = socket->SetMulticastTimeToLive(
          socket_options->multicast_time_to_live);
    }
    if (result == net::OK) {
      result = socket->SetMulticastLoopbackMode(
          socket_options->multicast_loopback_mode);
    }
  }
  net::IPEndPoint local_addr_out;
  if (result == net::OK) {
    result = socket->Bind(local_addr, &local_addr_out);
  }
  if (result != net::OK) {
    if (receiver)
      receiver.reset();
    std::move(callback).Run(result, base::nullopt);
    return;
  }
  // base::Unretained is safe as the destruction of |this| will also destroy
  // |udp_sockets_| which owns this socket.
  socket->set_connection_error_handler(
      base::BindOnce(&UDPSocketFactory::OnPipeBroken, base::Unretained(this),
                     base::Unretained(socket.get())));
  udp_sockets_.push_back(std::move(socket));
  std::move(callback).Run(result, local_addr_out);
}

void UDPSocketFactory::OpenAndConnect(
    network::mojom::UDPSocketRequest request,
    mojom::UDPSocketOptionsPtr socket_options,
    network::mojom::UDPSocketReceiverPtr receiver,
    const net::IPEndPoint& remote_addr,
    OpenAndConnectCallback callback) {
  auto socket =
      std::make_unique<UDPSocket>(std::move(request), std::move(receiver));
  int result = socket->Open(remote_addr.GetFamily());
  if (result != net::OK) {
    if (receiver)
      receiver.reset();
    std::move(callback).Run(result, base::nullopt);
    return;
  }
  net::IPEndPoint local_addr;
  result = socket->Connect(remote_addr, &local_addr);
  if (result != net::OK) {
    receiver.reset();
    std::move(callback).Run(result, base::nullopt);
    return;
  }
  // base::Unretained is safe as the destruction of |this| will also destroy
  // |udp_sockets_| which owns this socket.
  socket->set_connection_error_handler(
      base::BindOnce(&UDPSocketFactory::OnPipeBroken, base::Unretained(this),
                     base::Unretained(socket.get())));
  udp_sockets_.push_back(std::move(socket));
  std::move(callback).Run(result, local_addr);
}

void UDPSocketFactory::OnPipeBroken(network::UDPSocket* socket) {
  udp_sockets_.erase(
      std::find_if(udp_sockets_.begin(), udp_sockets_.end(),
                   [socket](const std::unique_ptr<network::UDPSocket>& ptr) {
                     return ptr.get() == socket;
                   }));
}

}  // namespace network
