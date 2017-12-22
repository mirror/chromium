// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_UDP_SOCKET_H_
#define SERVICES_NETWORK_UDP_SOCKET_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/binding.h"
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

class UDPSocket : public mojom::UDPSocket {
 public:
  // A socket wrapper class that allows tests to substitute the default
  // implementation (implemented using net::UDPSocket) with a test
  // implementation.
  class SocketWrapper {
   public:
    virtual ~SocketWrapper() {}
    // This wrapper class forwards the functions to an concrete udp socket
    // implementation. Please refer to udp_socket_posix.h/udp_socket_win.h for
    // definitions.
    virtual int Open(net::AddressFamily address_family) = 0;
    virtual int Connect(const net::IPEndPoint& remote_addr) = 0;
    virtual int Bind(const net::IPEndPoint& local_addr) = 0;
    virtual int AllowAddressReuse() = 0;
    virtual int SetMulticastTimeToLive(int ttl) = 0;
    virtual int SetMulticastLoopbackMode(bool loopback) = 0;
    virtual int SetSendBufferSize(uint32_t size) = 0;
    virtual int SetReceiveBufferSize(uint32_t size) = 0;
    virtual int SetBroadcast(bool enabled) = 0;
    virtual int JoinGroup(const net::IPAddress& group_address) = 0;
    virtual int LeaveGroup(const net::IPAddress& group_address) = 0;
    virtual int SendTo(net::IOBuffer* buf,
                       int buf_len,
                       const net::IPEndPoint& dest_addr,
                       const net::CompletionCallback& callback) = 0;
    virtual int Write(net::IOBuffer* buf,
                      int buf_len,
                      const net::CompletionCallback& callback) = 0;
    virtual int RecvFrom(net::IOBuffer* buf,
                         int buf_len,
                         net::IPEndPoint* address,
                         const net::CompletionCallback& callback) = 0;
    virtual int GetLocalAddress(net::IPEndPoint* address) const = 0;
  };

  // Constructs a UDPSocket and binds to |request|. The |receiver| can be a null
  // interface pointer if consumer wants to ignore incoming data.
  explicit UDPSocket(mojom::UDPSocketRequest request,
                     mojom::UDPSocketReceiverPtr receiver);
  ~UDPSocket() override;

  // Sets connection error handler.
  void set_connection_error_handler(base::OnceClosure handler);

  // Opens the udp socket. Returns net::OK on success and a negative net error
  // code on failure.
  int Open(net::AddressFamily address_family);

  // Connects the udp socket to |remote_addr|. Returns net::OK on success and
  // writes the local address of the socket to |local_addr_out_|. Returns a
  // negative net error code on failure.
  int Connect(const net::IPEndPoint& remote_addr,
              net::IPEndPoint* local_addr_out);

  // Binds the udp socket to |local_addr|. Caller can use port 0 to let the OS
  // pick an available port.
  int Bind(const net::IPEndPoint& local_addr, net::IPEndPoint* local_addr_out);

  // Enable SO_REUSEADDR on the underlying socket. Should be called between
  // Open() and Bind().
  int SetAllowAddressReuse();

  // Sets the time-to-live option for UDP packets sent to the multicast
  // group address.
  // Should be called before Bind(). Returns a net error code.
  int SetMulticastTimeToLive(int ttl);

  // Sets the loopback flag for UDP socket. If this flag is true, the host
  // will receive packets sent to the joined group from itself.
  // The default value of this option is true. Should be called before Bind().
  // Returns a net error code.
  int SetMulticastLoopbackMode(bool loopback);

  // UDPSocket implementation.
  void SetSendBufferSize(uint32_t size,
                         SetSendBufferSizeCallback callback) override;
  void SetReceiveBufferSize(uint32_t size,
                            SetReceiveBufferSizeCallback callback) override;
  void SetBroadcast(bool enabled, SetBroadcastCallback callback) override;
  void JoinGroup(const net::IPAddress& group_address,
                 JoinGroupCallback callback) override;
  void LeaveGroup(const net::IPAddress& group_address,
                  LeaveGroupCallback callback) override;
  void ReceiveMore(uint32_t num_additional_datagrams) override;
  void SendTo(const net::IPEndPoint& dest_addr,
              base::span<const uint8_t> data,
              SendToCallback callback) override;
  void Send(base::span<const uint8_t> data, SendCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(UDPSocketTest, TestInsufficientResources);
  FRIEND_TEST_ALL_PREFIXES(UDPSocketTest, TestReadZeroByte);

  // Represents a pending Send()/SendTo() request that is yet to be sent to the
  // |socket_|. In the case of Send(), |addr| will be not filled in.
  struct PendingSendRequest {
    PendingSendRequest();
    ~PendingSendRequest();

    std::unique_ptr<net::IPEndPoint> addr;
    base::span<const uint8_t> data;
    SendToCallback callback;
  };

  void DoRecvFrom();
  void DoSendToOrWrite(const net::IPEndPoint* dest_addr,
                       const base::span<const uint8_t>& data,
                       SendToCallback callback);

  void OnRecvFromCompleted(int net_result);
  void OnSendToCompleted(int net_result);

  // Whether an Open() has been successfully executed.
  bool is_opened_;

  // Whether a Connect() or Bind() has been successfully executed.
  bool is_connected_;

  // The interface which gets data from fulfilled receive requests.
  mojom::UDPSocketReceiverPtr receiver_;

  std::unique_ptr<SocketWrapper> wrapped_socket_;

  // Non-null when there is a pending RecvFrom operation on socket.
  scoped_refptr<net::IOBuffer> recvfrom_buffer_;

  // Non-null when there is a pending Send/SendTo operation on socket.
  scoped_refptr<net::IOBuffer> send_buffer_;
  SendToCallback send_callback_;

  // The address of the sender of a received packet. This address might not be
  // filled if an error occurred during the receiving of the packet.
  net::IPEndPoint recvfrom_address_;

  // How many more packets the client side expects to receive.
  uint32_t remaining_recv_slots_;

  // The queue owns the PendingSendRequest instances.
  base::circular_deque<std::unique_ptr<PendingSendRequest>>
      pending_send_requests_;
  // The maximum size of the |pending_send_requests_| queue.
  uint32_t max_pending_send_requests_;

  mojo::Binding<mojom::UDPSocket> binding_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocket);
};

}  // namespace network

#endif  // SERVICES_NETWORK_UDP_SOCKET_H_
