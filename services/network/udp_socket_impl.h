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
#include "net/base/address_family.h"
#include "net/base/ip_endpoint.h"
#include "net/interfaces/address_family.mojom.h"
#include "net/interfaces/ip_endpoint.mojom.h"
#include "net/socket/udp_socket.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"

namespace net {
class IOBuffer;
}  // namespace net

namespace network {

class UDPSocketImpl : public mojom::UDPSocket {
 public:
  // A socket wrapper class that allows substituting the default implementation
  // (implemented using net::UDPSocket) with a custom socket implementation.
  class SocketWrapper {
   public:
    virtual ~SocketWrapper() {}
    // This wrapper class forwards the functions to an concrete udp socket
    // implementation. Please refer to udp_socket_posix.h/udp_socket_win.h for
    // definitions.
    virtual int Open(net::AddressFamily address_family) = 0;
    virtual int Connect(const net::IPEndPoint& remote_addr) = 0;
    virtual int Bind(const net::IPEndPoint& local_addr) = 0;
    virtual int SetSendBufferSize(uint32_t size) = 0;
    virtual int SetReceiveBufferSize(uint32_t size) = 0;
    virtual int SendTo(net::IOBuffer* buf,
                       int buf_len,
                       const net::IPEndPoint& dest_addr,
                       const net::CompletionCallback& callback) = 0;
    virtual int RecvFrom(net::IOBuffer* buf,
                         int buf_len,
                         net::IPEndPoint* address,
                         const net::CompletionCallback& callback) = 0;
    virtual int GetLocalAddress(net::IPEndPoint* address) const = 0;
  };
  explicit UDPSocketImpl(mojom::UDPSocketReceiverPtr receiver);
  ~UDPSocketImpl() override;

  // UDPSocket implementation.
  void Open(net::AddressFamily address_family, OpenCallback callback) override;
  void Connect(const net::IPEndPoint& remote_addr,
               ConnectCallback callback) override;
  void Bind(const net::IPEndPoint& local_addr, BindCallback callback) override;
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
  FRIEND_TEST_ALL_PREFIXES(UDPSocketTest, TestReadZeroByte);

  // Represents a pending SendTo() request that is yet to be sent to the
  // |socket_|.
  struct PendingSendRequest {
    PendingSendRequest();
    ~PendingSendRequest();

    net::IPEndPoint addr;
    scoped_refptr<net::WrappedIOBuffer> data;
    size_t data_length;
    SendToCallback callback;
  };

  void DoRecvFrom();
  void DoSendTo(const net::IPEndPoint& addr,
                scoped_refptr<net::IOBuffer> data,
                size_t data_length,
                SendToCallback callback);

  void OnRecvFromCompleted(int net_result);
  void OnSendToCompleted(int net_result);

  // The interface which gets data from fulfilled receive requests.
  mojom::UDPSocketReceiverPtr receiver_;

  // The underlying socket implementation.
  std::unique_ptr<SocketWrapper> socket_;

  // Non-null when there is a pending RecvFrom operation on socket.
  scoped_refptr<net::IOBuffer> recvfrom_buffer_;
  // Non-null when there is a pending SendTo operation on socket.
  scoped_refptr<net::IOBuffer> sendto_buffer_;

  SendToCallback sendto_callback_;

  // The address of the pending RecvFrom operation, if any.
  net::IPEndPoint recvfrom_address_;

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
