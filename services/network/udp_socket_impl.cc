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

// TODO(xunjieli): Default read buffer size is too big. Make it customizable.
const int kMaxReadSize = 128 * 1024;
const size_t kMaxPendingSendRequestsUpperbound = 128;
const size_t kDefaultMaxPendingSendRequests = 32;

class SocketWrapperImpl : public UDPSocketImpl::SocketWrapper {
 public:
  SocketWrapperImpl(net::DatagramSocket::BindType bind_type,
                    const net::RandIntCallback& rand_int_cb,
                    net::NetLog* net_log,
                    const net::NetLogSource& source)
      : socket_(bind_type, rand_int_cb, net_log, source) {}
  ~SocketWrapperImpl() override {}

  int Open(net::AddressFamily address_family) override {
    return socket_.Open(address_family);
  }
  int Connect(const net::IPEndPoint& remote_addr) override {
    return socket_.Connect(remote_addr);
  }
  int Bind(const net::IPEndPoint& local_addr) override {
    return socket_.Bind(local_addr);
  }
  int SetSendBufferSize(uint32_t size) override {
    return socket_.SetSendBufferSize(size);
  }
  int SetReceiveBufferSize(uint32_t size) override {
    return socket_.SetReceiveBufferSize(size);
  }
  int SendTo(net::IOBuffer* buf,
             int buf_len,
             const net::IPEndPoint& dest_addr,
             const net::CompletionCallback& callback) override {
    return socket_.SendTo(buf, buf_len, dest_addr, callback);
  }
  int RecvFrom(net::IOBuffer* buf,
               int buf_len,
               net::IPEndPoint* address,
               const net::CompletionCallback& callback) override {
    return socket_.RecvFrom(buf, buf_len, address, callback);
  }
  int GetLocalAddress(net::IPEndPoint* address) const override {
    return socket_.GetLocalAddress(address);
  }

 private:
  net::UDPSocket socket_;

  DISALLOW_COPY_AND_ASSIGN(SocketWrapperImpl);
};

}  // namespace

UDPSocketImpl::PendingSendRequest::PendingSendRequest() {}

UDPSocketImpl::PendingSendRequest::~PendingSendRequest() {}

UDPSocketImpl::UDPSocketImpl(mojom::UDPSocketReceiverPtr receiver)
    : receiver_(std::move(receiver)),
      socket_(
          std::make_unique<SocketWrapperImpl>(net::DatagramSocket::DEFAULT_BIND,
                                              net::RandIntCallback(),
                                              nullptr,
                                              net::NetLogSource())),
      remaining_recv_slots_(0),
      max_pending_send_requests_(kDefaultMaxPendingSendRequests) {}

UDPSocketImpl::~UDPSocketImpl() {}

void UDPSocketImpl::Open(net::AddressFamily address_family,
                         OpenCallback callback) {
  int result = socket_->Open(address_family);
  std::move(callback).Run(result);
}

void UDPSocketImpl::Connect(const net::IPEndPoint& remote_addr,
                            ConnectCallback callback) {
  int result = socket_->Connect(remote_addr);
  net::IPEndPoint local_addr_out;
  if (result == net::OK) {
    result = socket_->GetLocalAddress(&local_addr_out);
  }
  std::move(callback).Run(result, local_addr_out);
}

void UDPSocketImpl::Bind(const net::IPEndPoint& local_addr,
                         BindCallback callback) {
  int result = socket_->Bind(local_addr);
  net::IPEndPoint local_addr_out;
  if (result == net::OK) {
    result = socket_->GetLocalAddress(&local_addr_out);
  }
  std::move(callback).Run(result, local_addr_out);
}

void UDPSocketImpl::SetSendBufferSize(uint32_t size,
                                      SetSendBufferSizeCallback callback) {
  if (size > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
    size = std::numeric_limits<int32_t>::max();
  int net_result = socket_->SetSendBufferSize(static_cast<int32_t>(size));
  std::move(callback).Run(net_result);
}

void UDPSocketImpl::SetReceiveBufferSize(
    uint32_t size,
    SetReceiveBufferSizeCallback callback) {
  if (size > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
    size = std::numeric_limits<int32_t>::max();
  int net_result = socket_->SetReceiveBufferSize(static_cast<int32_t>(size));
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

void UDPSocketImpl::DoRecvFrom() {
  DCHECK(receiver_);
  DCHECK(!recvfrom_buffer_.get());
  DCHECK_GT(remaining_recv_slots_, 0u);

  recvfrom_buffer_ = new net::IOBuffer(kMaxReadSize);

  // base::Unretained(this) is safe because socket is owned by |this|.
  int net_result = socket_->RecvFrom(
      recvfrom_buffer_.get(), kMaxReadSize, &recvfrom_address_,
      base::BindRepeating(&UDPSocketImpl::OnRecvFromCompleted,
                          base::Unretained(this)));
  if (net_result != net::ERR_IO_PENDING)
    OnRecvFromCompleted(net_result);
}

void UDPSocketImpl::DoSendTo(const net::IPEndPoint& addr,
                             scoped_refptr<net::IOBuffer> data,
                             size_t data_length,
                             SendToCallback callback) {
  DCHECK(!sendto_buffer_.get());

  sendto_buffer_ = std::move(data);
  sendto_callback_ = std::move(callback);

  if (addr.GetFamily() == net::ADDRESS_FAMILY_UNSPECIFIED) {
    std::move(callback).Run(net::ERR_ADDRESS_INVALID);
    return;
  }
  // base::Unretained(this) is safe because socket is owned by |this|.
  int net_result =
      socket_->SendTo(sendto_buffer_.get(), data_length, addr,
                      base::BindRepeating(&UDPSocketImpl::OnSendToCompleted,
                                          base::Unretained(this)));
  if (net_result != net::ERR_IO_PENDING)
    OnSendToCompleted(net_result);
}

void UDPSocketImpl::OnRecvFromCompleted(int net_result) {
  DCHECK(recvfrom_buffer_.get());

  net::IPEndPoint net_address;
  base::Optional<base::span<const uint8_t>> data;
  if (net_result >= 0) {
    data = base::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(recvfrom_buffer_->data()),
        static_cast<size_t>(net_result));
  }
  receiver_->OnReceived(net_result >= 0 ? net::OK : net_result,
                        recvfrom_address_, data);
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
