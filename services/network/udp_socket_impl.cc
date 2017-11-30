// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/udp_socket_impl.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <limits>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/log/net_log.h"
#include "net/socket/datagram_socket.h"

namespace network {

namespace {

const int kMaxReadSize = 128 * 1024;
const size_t kMaxWriteSize = 128 * 1024;
const size_t kMaxPendingSendRequestsUpperbound = 128;
const size_t kDefaultMaxPendingSendRequests = 32;

}  // namespace

// static
int UDPSocketImpl::CreateClientSocket(mojom::UDPSocketRequest request,
                                      mojom::UDPSocketReceiverRequest* receiver,
                                      const net::IPEndPoint& remote_addr) {
  auto* impl = new UDPSocketImpl(/*is_client_socket=*/true,
                                 /*allow_address_reuse=*/false);
  auto binding =
      mojo::MakeStrongBinding(base::WrapUnique(impl), std::move(request));
  impl->binding_ = binding;
  int net_result = impl->Connect(remote_addr);
  if (net_result == net::OK)
    *receiver = mojo::MakeRequest(&impl->receiver_);
  return net_result;
}

// static
int UDPSocketImpl::CreateServerSocket(mojom::UDPSocketRequest request,
                                      mojom::UDPSocketReceiverRequest* receiver,
                                      const net::IPEndPoint& local_addr,
                                      bool allow_address_reuse) {
  auto* impl =
      new UDPSocketImpl(/*is_client_socket=*/false, allow_address_reuse);
  auto binding =
      mojo::MakeStrongBinding(base::WrapUnique(impl), std::move(request));
  impl->binding_ = binding;
  int net_result = impl->Bind(local_addr);
  if (net_result == net::OK)
    *receiver = mojo::MakeRequest(&impl->receiver_);
  return net_result;
}

UDPSocketImpl::PendingSendRequest::PendingSendRequest() {}

UDPSocketImpl::PendingSendRequest::~PendingSendRequest() {}

UDPSocketImpl::UDPSocketImpl(bool is_client_socket, bool allow_address_reuse)
    : is_client_socket_(is_client_socket),
      state_(NOT_BOUND_OR_CONNECTED),
      allow_address_reuse_(allow_address_reuse),
      remaining_recv_slots_(0),
      max_pending_send_requests_(kDefaultMaxPendingSendRequests) {
  if (is_client_socket_) {
    client_socket_ = std::make_unique<net::UDPClientSocket>(
        net::DatagramSocket::DEFAULT_BIND, net::RandIntCallback(), nullptr,
        net::NetLogSource());
  } else {
    server_socket_ =
        std::make_unique<net::UDPServerSocket>(nullptr, net::NetLogSource());
  }
}

UDPSocketImpl::~UDPSocketImpl() {
  for (auto* request : pending_send_requests_) {
    delete request;
  }
}

int UDPSocketImpl::Bind(const net::IPEndPoint& local_addr) {
  DCHECK(!is_client_socket_);
  DCHECK(!IsBoundOrConnected());

  if (allow_address_reuse_)
    server_socket_->AllowAddressReuse();

  int net_result = server_socket_->Listen(local_addr);
  if (net_result == net::OK)
    state_ = BOUND;
  return net_result;
}

int UDPSocketImpl::Connect(const net::IPEndPoint& remote_addr) {
  DCHECK(is_client_socket_);
  DCHECK(!IsBoundOrConnected());

  int net_result = client_socket_->Connect(remote_addr);
  if (net_result == net::OK)
    state_ = CONNECTED;
  return net_result;
}

void UDPSocketImpl::SetSendBufferSize(uint32_t size,
                                      SetSendBufferSizeCallback callback) {
  DCHECK(IsBoundOrConnected());

  if (size > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
    size = std::numeric_limits<int32_t>::max();

  int net_result =
      is_client_socket_
          ? client_socket_->SetSendBufferSize(static_cast<int32_t>(size))
          : server_socket_->SetSendBufferSize(static_cast<int32_t>(size));
  std::move(callback).Run(net_result);
}

void UDPSocketImpl::SetReceiveBufferSize(
    uint32_t size,
    SetReceiveBufferSizeCallback callback) {
  DCHECK(IsBoundOrConnected());

  if (size > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
    size = std::numeric_limits<int32_t>::max();

  int net_result =
      is_client_socket_
          ? client_socket_->SetReceiveBufferSize(static_cast<int32_t>(size))
          : server_socket_->SetReceiveBufferSize(static_cast<int32_t>(size));
  std::move(callback).Run(net_result);
}

void UDPSocketImpl::NegotiateMaxPendingSendRequests(
    uint32_t requested_size,
    NegotiateMaxPendingSendRequestsCallback callback) {
  if (requested_size != 0) {
    max_pending_send_requests_ = std::min(kMaxPendingSendRequestsUpperbound,
                                          static_cast<size_t>(requested_size));
  }
  std::move(callback).Run(static_cast<uint32_t>(max_pending_send_requests_));

  while (pending_send_requests_.size() > max_pending_send_requests_) {
    std::deque<PendingSendRequest*> discarded_requests(
        pending_send_requests_.begin() + max_pending_send_requests_,
        pending_send_requests_.end());
    pending_send_requests_.resize(max_pending_send_requests_);
    for (auto* discarded_request : discarded_requests) {
      std::move(discarded_request->callback)
          .Run(net::ERR_INSUFFICIENT_RESOURCES);
      delete discarded_request;
    }
  }
}

void UDPSocketImpl::ReceiveMore(uint32_t num_additional_datagrams) {
  if (!receiver_)
    return;
  if (num_additional_datagrams == 0)
    return;
  if (std::numeric_limits<size_t>::max() - remaining_recv_slots_ <
      num_additional_datagrams) {
    return;
  }

  remaining_recv_slots_ += num_additional_datagrams;

  if (IsBoundOrConnected() && !recvfrom_buffer_.get()) {
    DCHECK_EQ(num_additional_datagrams, remaining_recv_slots_);
    DoRecvFrom();
  }
}

void UDPSocketImpl::SendTo(const net::IPEndPoint& dest_addr,
                           const std::vector<uint8_t>& data,
                           SendToCallback callback) {
  if (!IsBoundOrConnected()) {
    std::move(callback).Run(net::ERR_FAILED);
    return;
  }
  if (sendto_buffer_.get()) {
    if (pending_send_requests_.size() >= max_pending_send_requests_) {
      std::move(callback).Run(net::ERR_INSUFFICIENT_RESOURCES);
      return;
    }

    PendingSendRequest* request = new PendingSendRequest;
    request->addr = std::move(dest_addr);
    request->data = std::move(data);
    request->callback = std::move(callback);
    pending_send_requests_.push_back(request);
    return;
  }

  DCHECK_EQ(0u, pending_send_requests_.size());

  DoSendTo(std::move(dest_addr), std::move(data), std::move(callback));
}

void UDPSocketImpl::GetLocalAddress(GetLocalAddressCallback callback) {
  net::IPEndPoint local_addr;
  if (is_client_socket_) {
    client_socket_->GetLocalAddress(&local_addr);
  } else {
    server_socket_->GetLocalAddress(&local_addr);
  }

  std::move(callback).Run(local_addr);
}

void UDPSocketImpl::DoRecvFrom() {
  DCHECK(IsBoundOrConnected());
  DCHECK(receiver_);
  DCHECK(!recvfrom_buffer_.get());
  DCHECK_GT(remaining_recv_slots_, 0u);

  recvfrom_buffer_ = new net::IOBuffer(kMaxReadSize);

  // base::Unretained(this) is safe because socket is owned by |this|.
  int net_result;
  if (is_client_socket_) {
    net_result =
        client_socket_->Read(recvfrom_buffer_.get(), kMaxReadSize,
                             base::Bind(&UDPSocketImpl::OnRecvFromCompleted,
                                        base::Unretained(this)));
  } else {
    net_result = server_socket_->RecvFrom(
        recvfrom_buffer_.get(), kMaxReadSize, &recvfrom_address_,
        base::Bind(&UDPSocketImpl::OnRecvFromCompleted,
                   base::Unretained(this)));
  }
  if (net_result != net::ERR_IO_PENDING)
    OnRecvFromCompleted(net_result);
}

void UDPSocketImpl::DoSendTo(const net::IPEndPoint& addr,
                             const std::vector<uint8_t>& data,
                             SendToCallback callback) {
  DCHECK(IsBoundOrConnected());
  DCHECK(!sendto_buffer_.get());

  if (data.size() > kMaxWriteSize) {
    std::move(callback).Run(net::ERR_INVALID_ARGUMENT);
    return;
  }
  sendto_buffer_ = new net::IOBufferWithSize(static_cast<int>(data.size()));
  sendto_callback_ = std::move(callback);

  if (data.size() > 0)
    memcpy(sendto_buffer_->data(), data.data(), data.size());

  int net_result = net::OK;
  if (!is_client_socket_) {
    if (addr.GetFamily() == net::ADDRESS_FAMILY_UNSPECIFIED) {
      std::move(callback).Run(net::ERR_ADDRESS_INVALID);
      return;
    }

    // base::Unretained(this) is safe because socket is owned by |this|.
    net_result = server_socket_->SendTo(
        sendto_buffer_.get(), sendto_buffer_->size(), addr,
        base::Bind(&UDPSocketImpl::OnSendToCompleted, base::Unretained(this)));
  } else {
    DCHECK(state_ == CONNECTED);
    net_result = client_socket_->Write(
        sendto_buffer_.get(), sendto_buffer_->size(),
        base::Bind(&UDPSocketImpl::OnSendToCompleted, base::Unretained(this)));
  }
  if (net_result != net::ERR_IO_PENDING)
    OnSendToCompleted(net_result);
}

void UDPSocketImpl::OnRecvFromCompleted(int net_result) {
  DCHECK(recvfrom_buffer_.get());

  net::IPEndPoint net_address;
  std::vector<uint8_t> array;
  if (net_result >= 0) {
    if (state_ == BOUND)
      net_address = recvfrom_address_;

    std::vector<uint8_t> data(net_result);
    if (net_result > 0)
      memcpy(&data[0], recvfrom_buffer_->data(), net_result);

    std::swap(array, data);
  }
  recvfrom_buffer_ = nullptr;

  receiver_->OnReceived(net_result, std::move(net_address), std::move(array));
  DCHECK_GT(remaining_recv_slots_, 0u);
  remaining_recv_slots_--;
  if (remaining_recv_slots_ > 0)
    DoRecvFrom();
}

void UDPSocketImpl::OnSendToCompleted(int net_result) {
  DCHECK(sendto_buffer_.get());
  DCHECK(!sendto_callback_.is_null());

  sendto_buffer_ = nullptr;

  std::move(sendto_callback_).Run(net_result);

  if (pending_send_requests_.empty())
    return;

  auto request =
      base::WrapUnique<PendingSendRequest>(pending_send_requests_.front());
  pending_send_requests_.pop_front();

  DoSendTo(request->addr, std::move(request->data),
           std::move(request->callback));
}

}  // namespace network
