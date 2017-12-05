// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_UDP_SOCKET_IMPL_H_
#define SERVICES_NETWORK_UDP_SOCKET_IMPL_H_

#include <deque>
#include <memory>
#include <vector>

#include "base/containers/span.h"
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
}  // namespace net

namespace network {

class UDPSocketImpl : public mojom::UDPSocket {
 public:
  // Creates a client socket. |remote_addr| is the address of the remote to
  // connect to. If connect is successful, returns net::OK and |local_addr_out|
  // contains the local address of this socket. Otherwise, returns a net error
  // code indicating the error that occurred.
  static int CreateClientSocket(mojom::UDPSocketRequest request,
                                mojom::UDPSocketReceiverPtr receiver,
                                const net::IPEndPoint& remote_addr,
                                net::IPEndPoint* local_addr_out);

  // Creates a server socket. |local_address| is the desired local address of
  // the server socket. Upon success, returns net::OK and writes out the real
  // local address to |local_addr_out|. Otherwise, returns a net error code
  // indicating the error that occurred.
  static int CreateServerSocket(mojom::UDPSocketRequest request,
                                mojom::UDPSocketReceiverPtr receiver,
                                const net::IPEndPoint& local_addr,
                                net::IPEndPoint* local_addr_out,
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
              base::span<const uint8_t> data,
              SendToCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(UDPSocketTest, TestInsufficientResources);

  struct PendingSendRequest {
    PendingSendRequest();
    ~PendingSendRequest();

    net::IPEndPoint addr;
    scoped_refptr<net::WrappedIOBuffer> data;
    size_t data_length;
    SendToCallback callback;
  };

  // static
  static int CreateHelper(mojom::UDPSocketRequest request,
                          mojom::UDPSocketReceiverPtr receiver,
                          const net::IPEndPoint& addr,
                          net::IPEndPoint* local_addr_out,
                          bool is_client_socket,
                          bool allow_address_reuse);

  UDPSocketImpl(bool is_client_socket, mojom::UDPSocketReceiverPtr receiver);

  void DoRecvFrom();
  void DoSendTo(const net::IPEndPoint& addr,
                scoped_refptr<net::IOBuffer> data,
                size_t data_length,
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
  scoped_refptr<net::IOBuffer> sendto_buffer_;

  SendToCallback sendto_callback_;

  // The address of the pending RecvFrom operation, if any.
  net::IPEndPoint recvfrom_address_;

  // The interface which gets data from fulfilled receive requests.
  mojom::UDPSocketReceiverPtr receiver_;

  // How many more packets the client side expects to receive.
  size_t remaining_recv_slots_;

  // The queue owns the PendingSendRequest instances.
  base::circular_deque<std::unique_ptr<PendingSendRequest>>
      pending_send_requests_;
  // The maximum size of the |pending_send_requests_| queue.
  size_t max_pending_send_requests_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketImpl);
};

}  // namespace network

#endif  // SERVICES_NETWORK_UDP_SOCKET_IMPL_H_
