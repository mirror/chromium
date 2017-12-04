// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/udp_socket_impl.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/log/net_log.h"

namespace network {

namespace {

// TODO(xunjieli): The current read buffer size is too big.
const int kMaxReadSize = 128 * 1024;
const size_t kMaxWriteSize = 128 * 1024;
const size_t kMaxPendingSendRequestsUpperbound = 128;
const size_t kDefaultMaxPendingSendRequests = 32;

}  // namespace

// static
int UDPSocketImpl::CreateServerSocket(mojom::UDPSocketRequest request,
                                      mojom::UDPSocketReceiverPtr receiver,
                                      const net::IPEndPoint& local_addr,
                                      net::IPEndPoint* local_addr_out,
                                      bool allow_address_reuse) {
  return CreateHelper(std::move(request), std::move(receiver), local_addr,
                      local_addr_out, false /*is_client_socket*/,
                      allow_address_reuse);
}

// static
int UDPSocketImpl::CreateClientSocket(mojom::UDPSocketRequest request,
                                      mojom::UDPSocketReceiverPtr receiver,
                                      const net::IPEndPoint& remote_addr,
                                      net::IPEndPoint* local_addr_out) {
  return CreateHelper(std::move(request), std::move(receiver), remote_addr,
                      local_addr_out, true /*is_client_socket*/,
                      false /*allow_address_reuse*/);
}
UDPSocketImpl::PendingSendRequest::PendingSendRequest() {}

UDPSocketImpl::PendingSendRequest::~PendingSendRequest() {}

UDPSocketImpl::~UDPSocketImpl() {}

void UDPSocketImpl::SetSendBufferSize(uint32_t size,
                                      SetSendBufferSizeCallback callback) {
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
    PendingSendRequest* discarded_request = pending_send_requests_.back().get();
    std::move(discarded_request->callback).Run(net::ERR_INSUFFICIENT_RESOURCES);
    pending_send_requests_.pop_back();
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

  if (!recvfrom_buffer_.get()) {
    DCHECK_EQ(num_additional_datagrams, remaining_recv_slots_);
    DoRecvFrom();
  }
}

void UDPSocketImpl::SendTo(const net::IPEndPoint& dest_addr,
                           base::span<const uint8_t> data,
                           SendToCallback callback) {
  if (sendto_buffer_.get()) {
    if (pending_send_requests_.size() >= max_pending_send_requests_) {
      std::move(callback).Run(net::ERR_INSUFFICIENT_RESOURCES);
      return;
    }

    auto request = std::make_unique<PendingSendRequest>();
    request->addr = std::move(dest_addr);
    request->data =
        new net::WrappedIOBuffer(reinterpret_cast<const char*>(data.data()));
    request->data_length = data.size();
    request->callback = std::move(callback);
    pending_send_requests_.push_back(std::move(request));
    return;
  }

  DCHECK_EQ(0u, pending_send_requests_.size());

  DoSendTo(std::move(dest_addr),
           new net::WrappedIOBuffer(reinterpret_cast<const char*>(data.data())),
           data.size(), std::move(callback));
}

// static
int UDPSocketImpl::CreateHelper(mojom::UDPSocketRequest request,
                                mojom::UDPSocketReceiverPtr receiver,
                                const net::IPEndPoint& addr,
                                net::IPEndPoint* local_addr_out,
                                bool is_client_socket,
                                bool allow_address_reuse) {
  // Use base::WrapUnique instead of std::make_unique because the constructor
  // of UDPSocketImpl is private.
  auto impl = base::WrapUnique(
      new UDPSocketImpl(is_client_socket, std::move(receiver)));
  int net_result;
  if (is_client_socket) {
    net_result = impl->client_socket_->Connect(/*remote_addr=*/addr);
  } else {
    if (allow_address_reuse)
      impl->server_socket_->AllowAddressReuse();
    net_result = impl->server_socket_->Listen(/*local_addr=*/addr);
  }
  if (net_result != net::OK)
    return net_result;
  int get_local_address_result =
      is_client_socket ? impl->client_socket_->GetLocalAddress(local_addr_out)
                       : impl->server_socket_->GetLocalAddress(local_addr_out);
  if (get_local_address_result != net::OK)
    return get_local_address_result;
  mojo::MakeStrongBinding(std::move(impl), std::move(request));
  return net::OK;
}

UDPSocketImpl::UDPSocketImpl(bool is_client_socket,
                             mojom::UDPSocketReceiverPtr receiver)
    : is_client_socket_(is_client_socket),
      receiver_(std::move(receiver)),
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

void UDPSocketImpl::DoRecvFrom() {
  DCHECK(receiver_);
  DCHECK(!recvfrom_buffer_.get());
  DCHECK_GT(remaining_recv_slots_, 0u);

  recvfrom_buffer_ = new net::IOBuffer(kMaxReadSize);

  // base::Unretained(this) is safe because socket is owned by |this|.
  int net_result;
  if (is_client_socket_) {
    net_result = client_socket_->Read(
        recvfrom_buffer_.get(), kMaxReadSize,
        base::BindRepeating(&UDPSocketImpl::OnRecvFromCompleted,
                            base::Unretained(this)));
  } else {
    net_result = server_socket_->RecvFrom(
        recvfrom_buffer_.get(), kMaxReadSize, &recvfrom_address_,
        base::BindRepeating(&UDPSocketImpl::OnRecvFromCompleted,
                            base::Unretained(this)));
  }
  if (net_result != net::ERR_IO_PENDING)
    OnRecvFromCompleted(net_result);
}

void UDPSocketImpl::DoSendTo(const net::IPEndPoint& addr,
                             scoped_refptr<net::IOBuffer> data,
                             size_t data_length,
                             SendToCallback callback) {
  DCHECK(!sendto_buffer_.get());

  if (data_length > kMaxWriteSize) {
    std::move(callback).Run(net::ERR_INVALID_ARGUMENT);
    return;
  }
  sendto_buffer_ = std::move(data);
  sendto_callback_ = std::move(callback);

  int net_result = net::OK;
  if (is_client_socket_) {
    net_result = client_socket_->Write(
        sendto_buffer_.get(), data_length,
        base::BindRepeating(&UDPSocketImpl::OnSendToCompleted,
                            base::Unretained(this)));
  } else {
    if (addr.GetFamily() == net::ADDRESS_FAMILY_UNSPECIFIED) {
      std::move(callback).Run(net::ERR_ADDRESS_INVALID);
      return;
    }
    // base::Unretained(this) is safe because socket is owned by |this|.
    net_result = server_socket_->SendTo(
        sendto_buffer_.get(), data_length, addr,
        base::BindRepeating(&UDPSocketImpl::OnSendToCompleted,
                            base::Unretained(this)));
  }
  if (net_result != net::ERR_IO_PENDING)
    OnSendToCompleted(net_result);
}

void UDPSocketImpl::OnRecvFromCompleted(int net_result) {
  DCHECK(recvfrom_buffer_.get());

  net::IPEndPoint net_address;
  base::Optional<base::span<const uint8_t>> data;
  if (net_result >= 0) {
    if (!is_client_socket_)
      net_address = recvfrom_address_;
    data = base::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(recvfrom_buffer_->data()),
        static_cast<size_t>(net_result));
  }
  receiver_->OnReceived(net_result, std::move(net_address),
                        net_result < 0 ? base::nullopt : data);
  recvfrom_buffer_ = nullptr;
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

  std::unique_ptr<PendingSendRequest> request =
      std::move(pending_send_requests_.front());
  pending_send_requests_.pop_front();
  DoSendTo(request->addr, request->data, request->data_length,
           std::move(request->callback));
}

}  // namespace network
