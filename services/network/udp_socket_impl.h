// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_UDP_SOCKET_IMPL_H_
#define SERVICES_NETWORK_UDP_SOCKET_IMPL_H_

#include <deque>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/ip_endpoint.h"
#include "net/interfaces/ip_endpoint.mojom.h"
#include "net/socket/udp_client_socket.h"
#include "net/socket/udp_server_socket.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"

namespace net {
class IOBuffer;
class IOBufferWithSize;
}  // namespace net

namespace network {

class UDPSocketImpl : public mojom::UDPSocket {
 public:
  static int CreateClientSocket(mojom::UDPSocketRequest request,
                                mojom::UDPSocketReceiverRequest* receiver,
                                const net::IPEndPoint& remote_addr);

  static int CreateServerSocket(mojom::UDPSocketRequest request,
                                mojom::UDPSocketReceiverRequest* receiver,
                                const net::IPEndPoint& local_addr,
                                bool allow_address_reuse);

  ~UDPSocketImpl() override;

  // UDPSocket implementation.
  void SetSendBufferSize(uint32_t size,
                         SetSendBufferSizeCallback callback) override;

  void SetReceiveBufferSize(uint32_t size,
                            SetReceiveBufferSizeCallback callback) override;

  void NegotiateMaxPendingSendRequests(
      uint32_t requested_size,
      NegotiateMaxPendingSendRequestsCallback callback) override;

  void ReceiveMore(uint32_t num_additional_datagrams) override;

  void SendTo(const net::IPEndPoint& dest_addr,
              const std::vector<uint8_t>& data,
              SendToCallback callback) override;

  void GetLocalAddress(GetLocalAddressCallback callback) override;

 private:
  struct PendingSendRequest {
    PendingSendRequest();
    ~PendingSendRequest();

    net::IPEndPoint addr;
    std::vector<uint8_t> data;
    SendToCallback callback;
  };

  // static
  static int CreateHelper(mojom::UDPSocketRequest request,
                          mojom::UDPSocketReceiverRequest* receiver,
                          const net::IPEndPoint& addr,
                          bool is_client_socket,
                          bool allow_address_reuse);

  explicit UDPSocketImpl(bool is_client_socket);

  void DoRecvFrom();
  void DoSendTo(const net::IPEndPoint& addr,
                const std::vector<uint8_t>& data,
                SendToCallback callback);

  void OnRecvFromCompleted(int net_result);
  void OnSendToCompleted(int net_result);

  // True if this is a client socket. Otherwise, this is a server socket.
  const bool is_client_socket_;

  // The underlying socket implementation. Exactly one of |client_socket_| and
  // |server_socket_| is non-null but not both.
  std::unique_ptr<net::UDPClientSocket> client_socket_;
  std::unique_ptr<net::UDPServerSocket> server_socket_;

  // Non-null when there is a pending RecvFrom operation on socket.
  scoped_refptr<net::IOBuffer> recvfrom_buffer_;
  // Non-null when there is a pending SendTo operation on socket.
  scoped_refptr<net::IOBufferWithSize> sendto_buffer_;
  SendToCallback sendto_callback_;

  // The address of the pending RecvFrom operation, if any.
  net::IPEndPoint recvfrom_address_;

  // The interface which gets data from fulfilled receive requests.
  mojom::UDPSocketReceiverPtr receiver_;

  // How many more packets the client side expects to receive.
  size_t remaining_recv_slots_;

  // The queue owns the PendingSendRequest instances.
  std::deque<PendingSendRequest*> pending_send_requests_;
  // The maximum size of the |pending_send_requests_| queue.
  size_t max_pending_send_requests_;

  mojo::StrongBindingPtr<mojom::UDPSocket> binding_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketImpl);
};

}  // namespace network

#endif  // SERVICES_NETWORK_UDP_SOCKET_IMPL_H_
