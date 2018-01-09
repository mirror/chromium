// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_UDP_SOCKET_FACTORY_H_
#define SERVICES_NETWORK_UDP_SOCKET_FACTORY_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/base/ip_endpoint.h"
#include "net/interfaces/ip_endpoint.mojom.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"

namespace network {

class UDPSocket;

class UDPSocketFactory : public mojom::UDPSocketFactory {
 public:
  UDPSocketFactory();
  ~UDPSocketFactory() override;

  // Binds a UDPSockeFactory request to this object. Mojo messages
  // coming through the associated pipe will be served by this object.
  void AddRequest(mojom::UDPSocketFactoryRequest request);

  // mojom::UDPSocketFactory implementation.
  void OpenAndBind(mojom::UDPSocketRequest request,
                   mojom::UDPSocketOptionsPtr socket_options,
                   mojom::UDPSocketReceiverPtr receiver,
                   const net::IPEndPoint& local_addr,
                   OpenAndConnectCallback callback) override;
  void OpenAndConnect(mojom::UDPSocketRequest request,
                      mojom::UDPSocketOptionsPtr socket_options,
                      mojom::UDPSocketReceiverPtr receiver,
                      const net::IPEndPoint& remote_addr,
                      OpenAndConnectCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(UDPSocketFactoryTest, ConnectionError);

  // Handles connection errors on notification pipes.
  void OnPipeBroken(network::UDPSocket* client);

  mojo::BindingSet<mojom::UDPSocketFactory> bindings_;
  std::vector<std::unique_ptr<network::UDPSocket>> udp_sockets_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_UDP_SOCKET_FACTORY_H_
