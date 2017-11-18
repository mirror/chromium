// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/chromium/quic_connectivity_probing_manager.h"

namespace net {

namespace {

// Default to 2 seconds timeout as the maximum timeout.
const int64_t kMaxProbingTimeoutMs = 2000;

}  // namespace

QuicConnectivityProbingManager::QuicConnectivityProbingManager(
    Delegate* delegate,
    base::SequencedTaskRunner* task_runner)
    : delegate_(delegate),
      retry_count_(0),
      task_runner_(task_runner),
      weak_factory_(this) {
  retransmit_timer_.SetTaskRunner(task_runner_);
}

QuicConnectivityProbingManager::~QuicConnectivityProbingManager() {
  CancelProbing();
}

void QuicConnectivityProbingManager::CancelProbing() {
  network_ = NetworkChangeNotifier::kInvalidNetworkHandle;
  peer_address_ = QuicSocketAddress();
  socket_.reset();
  writer_.reset();
  reader_.reset();
  retry_count_ = 0;
  initial_timeout_ = base::TimeDelta();
  retransmit_timer_.Stop();
}

void QuicConnectivityProbingManager::StartProbing(
    NetworkChangeNotifier::NetworkHandle network,
    const QuicSocketAddress& peer_address,
    std::unique_ptr<DatagramClientSocket> socket,
    std::unique_ptr<QuicChromiumPacketWriter> writer,
    std::unique_ptr<QuicChromiumPacketReader> reader,
    base::TimeDelta initial_timeout) {
  // Start a new probe will always cancel the previous one.
  CancelProbing();

  network_ = network;
  peer_address_ = peer_address;
  socket_ = std::move(socket);
  writer_ = std::move(writer);
  reader_ = std::move(reader);
  initial_timeout_ = initial_timeout;

  reader_->StartReading();
  SendConnectivityProbingPacket(initial_timeout_);
}

void QuicConnectivityProbingManager::OnConnectivityProbingReceived(
    const QuicSocketAddress& self_address,
    const QuicSocketAddress& peer_address) {
  if (!socket_) {
    DVLOG(1) << "Probing response is ignored as probing was cancelled "
             << "or succeeded.";
    return;
  }

  IPEndPoint local_address;
  socket_->GetLocalAddress(&local_address);

  DVLOG(1) << "Current probing is live at self ip:port "
           << local_address.ToString() << ", to peer ip:port "
           << peer_address_.ToString();

  if (QuicSocketAddressImpl(local_address) != self_address.impl() ||
      peer_address_ != peer_address) {
    DVLOG(1) << "Received probing response from peer ip:port "
             << peer_address.ToString() << ", to self ip:port "
             << self_address.ToString() << ". Ignored.";
    return;
  }

  // TODO(zhongyi): add metrics collection.
  // Notify the delegate that the probe succeeds and reset everything.
  delegate_->OnProbeNetworkSucceeded(network_, self_address, std::move(socket_),
                                     std::move(writer_), std::move(reader_));
  CancelProbing();
}

void QuicConnectivityProbingManager::SendConnectivityProbingPacket(
    base::TimeDelta timeout) {
  delegate_->OnSendConnectivityProbingPacket(writer_.get(), peer_address_);
  retransmit_timer_.Start(
      FROM_HERE, timeout,
      base::Bind(
          &QuicConnectivityProbingManager::MaybeResendConnectivityProbingPacket,
          weak_factory_.GetWeakPtr()));
}

void QuicConnectivityProbingManager::MaybeResendConnectivityProbingPacket() {
  // Use exponential backoff for the timeout
  retry_count_++;
  int64_t timeout_ms = (1 << retry_count_) * initial_timeout_.InMilliseconds();
  if (timeout_ms > kMaxProbingTimeoutMs) {
    delegate_->OnProbeNetworkFailed(network_);
    CancelProbing();
    return;
  }
  SendConnectivityProbingPacket(base::TimeDelta::FromMilliseconds(timeout_ms));
}

}  // namespace net
