// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/socket/udp_socket.h"

#include <algorithm>

#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/stl_util.h"
#include "extensions/browser/api/api_resource.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/datagram_socket.h"
#include "net/socket/udp_client_socket.h"

namespace extensions {

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ApiResourceManager<ResumableUDPSocket>>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
template <>
BrowserContextKeyedAPIFactory<ApiResourceManager<ResumableUDPSocket> >*
ApiResourceManager<ResumableUDPSocket>::GetFactoryInstance() {
  return g_factory.Pointer();
}

UDPSocket::UDPSocket(network::mojom::UDPSocketFactoryPtr factory,
                     const std::string& owner_extension_id)
    : Socket(owner_extension_id),
      socket_factory_(std::move(factory)),
      socket_options_(network::mojom::UDPSocketOptions::New()),
      receiver_binding_(this) {}

UDPSocket::~UDPSocket() {
  Disconnect(true /* socket_destroying */);
}

void UDPSocket::Connect(const net::AddressList& address,
                        const net::CompletionCallback& callback) {
  int result = net::ERR_CONNECTION_FAILED;
  if (is_connected_) {
    callback.Run(result);
    return;
  }
  // UDP API only connects to the first address received from DNS so
  // connection may not work even if other addresses are reachable.
  const net::IPEndPoint& ip_end_point = address.front();
  network::mojom::UDPSocketReceiverPtr receiver_ptr;
  receiver_binding_.Bind(mojo::MakeRequest(&receiver_ptr));
  socket_factory_->OpenAndConnect(
      mojo::MakeRequest(&socket_), std::move(socket_options_),
      std::move(receiver_ptr), ip_end_point,
      base::BindOnce(&UDPSocket::OnConnectOrBindComplete,
                     base::Unretained(this), callback));
}

void UDPSocket::Bind(const std::string& address,
                     uint16_t port,
                     const net::CompletionCallback& callback) {
  if (is_connected_) {
    callback.Run(net::ERR_CONNECTION_FAILED);
    return;
  }

  net::IPEndPoint ip_end_point;
  if (!StringAndPortToIPEndPoint(address, port, &ip_end_point)) {
    callback.Run(net::ERR_INVALID_ARGUMENT);
    return;
  }
  network::mojom::UDPSocketReceiverPtr receiver_ptr;
  receiver_binding_.Bind(mojo::MakeRequest(&receiver_ptr));
  socket_factory_->OpenAndBind(
      mojo::MakeRequest(&socket_), std::move(socket_options_),
      std::move(receiver_ptr), ip_end_point,
      base::BindOnce(&UDPSocket::OnConnectOrBindComplete,
                     base::Unretained(this), callback));
}

void UDPSocket::Disconnect(bool socket_destroying) {
  is_connected_ = false;
  socket_.reset();
  read_callback_.Reset();
  // TODO(devlin): Should we do this for all callbacks?
  if (!recv_from_callback_.is_null()) {
    base::ResetAndReturn(&recv_from_callback_)
        .Run(net::ERR_CONNECTION_CLOSED, nullptr, true /* socket_destroying */,
             std::string(), 0);
  }
  send_to_callback_.Reset();
  multicast_groups_.clear();
}

void UDPSocket::Read(int count, const ReadCompletionCallback& callback) {
  DCHECK(!callback.is_null());

  if (!read_callback_.is_null()) {
    callback.Run(net::ERR_IO_PENDING, nullptr, false /* socket_destroying */);
    return;
  }

  if (count < 0) {
    callback.Run(net::ERR_INVALID_ARGUMENT, nullptr,
                 false /* socket_destroying */);
    return;
  }

  if (!IsConnected()) {
    callback.Run(net::ERR_SOCKET_NOT_CONNECTED, nullptr,
                 false /* socket_destroying */);
    return;
  }

  read_callback_ = callback;
  // FIXME: make use of |count|.
  socket_->ReceiveMore(1);
  return;
}

int UDPSocket::WriteImpl(net::IOBuffer* io_buffer,
                         int io_buffer_size,
                         const net::CompletionCallback& callback) {
  if (!IsConnected()) {
    return net::ERR_SOCKET_NOT_CONNECTED;
  } else {
    base::span<const uint8_t> data(
        reinterpret_cast<const uint8_t*>(io_buffer->data()),
        static_cast<size_t>(io_buffer_size));
    // FIXME: plumb socket traffic tag.
    socket_->Send(data, base::BindOnce(&UDPSocket::OnWriteComplete,
                                       base::Unretained(this), callback));
    return net::ERR_IO_PENDING;
  }
}

void UDPSocket::RecvFrom(int count,
                         const RecvFromCompletionCallback& callback) {
  DCHECK(!callback.is_null());

  if (!recv_from_callback_.is_null()) {
    callback.Run(net::ERR_IO_PENDING, nullptr, false /* socket_destroying */,
                 std::string(), 0);
    return;
  }

  if (count < 0) {
    callback.Run(net::ERR_INVALID_ARGUMENT, nullptr,
                 false /* socket_destroying */, std::string(), 0);
    return;
  }

  if (!IsConnected()) {
    callback.Run(net::ERR_SOCKET_NOT_CONNECTED, nullptr,
                 false /* socket_destroying */, std::string(), 0);
    return;
  }

  // FIXME: make use of |count|.
  recv_from_callback_ = callback;
  socket_->ReceiveMore(1);
}

void UDPSocket::SendTo(scoped_refptr<net::IOBuffer> io_buffer,
                       int byte_count,
                       const net::IPEndPoint& address,
                       const net::CompletionCallback& callback) {
  DCHECK(!callback.is_null());

  if (!send_to_callback_.is_null()) {
    // TODO(penghuang): Put requests in a pending queue to support multiple
    // sendTo calls.
    callback.Run(net::ERR_IO_PENDING);
    return;
  } else {
    send_to_callback_ = callback;
  }

  int result = net::ERR_FAILED;
  /**
do {
  if (!socket_.is_connected()) {
    result = net::ERR_SOCKET_NOT_CONNECTED;
    break;
  }

  result = socket_.SendTo(
      io_buffer.get(), byte_count, address,
      base::Bind(&UDPSocket::OnSendToComplete, base::Unretained(this)));
} while (false);
      */

  if (result != net::ERR_IO_PENDING)
    OnSendToComplete(result);
}

bool UDPSocket::IsConnected() { return is_connected_; }

bool UDPSocket::GetPeerAddress(net::IPEndPoint* address) {
  return 0;  //! socket_.GetPeerAddress(address);
}

bool UDPSocket::GetLocalAddress(net::IPEndPoint* address) {
  return 0;  //! socket_.GetLocalAddress(address);
}

Socket::SocketType UDPSocket::GetSocketType() const { return Socket::TYPE_UDP; }

void UDPSocket::OnReceived(int32_t result,
                           const base::Optional<net::IPEndPoint>& src_addr,
                           base::Optional<base::span<const uint8_t>> data) {
  DCHECK(!recv_from_callback_.is_null() || !read_callback_.is_null());

  net::IOBuffer* io_buffer = nullptr;
  if (result >= 0) {
    io_buffer = new net::IOBuffer(result);
    memcpy(io_buffer->data(), data.value().data(), result);
  }
  if (!read_callback_.is_null()) {
    read_callback_.Run(result, io_buffer, false /* socket_destroying */);
    read_callback_.Reset();
    return;
  }
  std::string ip;
  uint16_t port = 0;
  if (result > 0 && src_addr) {
    IPEndPointToStringAndPort(src_addr.value(), &ip, &port);
  }
  recv_from_callback_.Run(result, io_buffer, false /* socket_destroying */, ip,
                          port);
  recv_from_callback_.Reset();
}

void UDPSocket::OnWriteComplete(const net::CompletionCallback& user_callback,
                                int result) {
  user_callback.Run(result);
}

void UDPSocket::OnSendToComplete(int result) {
  DCHECK(!send_to_callback_.is_null());
  send_to_callback_.Run(result);
  send_to_callback_.Reset();
}

// FIXME: reorder to match header file.
void UDPSocket::OnJoinGroupComplete(
    const net::CompletionCallback& user_callback,
    const std::string& normalized_address,
    int result) {
  if (result == net::OK)
    multicast_groups_.push_back(normalized_address);
  user_callback.Run(result);
}

void UDPSocket::OnLeaveGroupComplete(
    const net::CompletionCallback& user_callback,
    const std::string& normalized_address,
    int result) {
  if (result == net::OK) {
    std::vector<std::string>::iterator find_result = std::find(
        multicast_groups_.begin(), multicast_groups_.end(), normalized_address);
    multicast_groups_.erase(find_result);
  }

  user_callback.Run(result);
}

void UDPSocket::OnConnectOrBindComplete(
    const net::CompletionCallback& user_callback,
    int result,
    const base::Optional<net::IPEndPoint>& local_addr) {
  if (result == net::OK) {
    local_addr_ = local_addr.value();
    is_connected_ = true;
  }
  user_callback.Run(result);
}

void UDPSocket::JoinGroup(const std::string& address,
                          const net::CompletionCallback& callback) {
  net::IPAddress ip;
  if (!ip.AssignFromIPLiteral(address)) {
    callback.Run(net::ERR_ADDRESS_INVALID);
    return;
  }

  std::string normalized_address = ip.ToString();
  if (base::ContainsValue(multicast_groups_, normalized_address)) {
    callback.Run(net::ERR_ADDRESS_INVALID);
    return;
  }

  socket_->JoinGroup(
      ip, base::BindOnce(&UDPSocket::OnJoinGroupComplete,
                         base::Unretained(this), callback, normalized_address));
}

void UDPSocket::LeaveGroup(const std::string& address,
                           const net::CompletionCallback& callback) {
  net::IPAddress ip;
  if (!ip.AssignFromIPLiteral(address)) {
    callback.Run(net::ERR_ADDRESS_INVALID);
    return;
  }

  std::string normalized_address = ip.ToString();
  std::vector<std::string>::iterator find_result = std::find(
      multicast_groups_.begin(), multicast_groups_.end(), normalized_address);
  if (find_result == multicast_groups_.end()) {
    callback.Run(net::ERR_ADDRESS_INVALID);
    return;
  }

  socket_->LeaveGroup(
      ip, base::Bind(&UDPSocket::OnLeaveGroupComplete, base::Unretained(this),
                     callback, normalized_address));
}

int UDPSocket::SetMulticastTimeToLive(int ttl) {
  // FIXME: more graceful error handling.
  if (!socket_options_) {
    return net::ERR_INVALID_ARGUMENT;
  }
  socket_options_->multicast_time_to_live = ttl;
  return net::OK;
}

int UDPSocket::SetMulticastLoopbackMode(bool loopback) {
  // FIXME: more graceful error handling.
  if (!socket_options_) {
    return net::ERR_INVALID_ARGUMENT;
  }
  socket_options_->multicast_loopback_mode = loopback;
  return net::OK;
}

void UDPSocket::SetBroadcast(bool enabled,
                             const net::CompletionCallback& callback) {
  if (!IsConnected()) {
    callback.Run(net::ERR_SOCKET_NOT_CONNECTED);
    return;
  }
  socket_->SetBroadcast(enabled, callback);
}

const std::vector<std::string>& UDPSocket::GetJoinedGroups() const {
  return multicast_groups_;
}

ResumableUDPSocket::ResumableUDPSocket(
    network::mojom::UDPSocketFactoryPtr factory,
    const std::string& owner_extension_id)
    : UDPSocket(std::move(factory), owner_extension_id),
      persistent_(false),
      buffer_size_(0),
      paused_(false) {}

bool ResumableUDPSocket::IsPersistent() const { return persistent(); }

}  // namespace extensions
