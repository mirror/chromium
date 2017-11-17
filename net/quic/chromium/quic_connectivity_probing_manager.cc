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
    Delegate* delegate)
    : delegate_(delegate),
      retry_count_(0),
      initial_timeout_ms_(0),
      weak_factory_(this) {}

QuicConnectivityProbingManager::~QuicConnectivityProbingManager() {
  CancelProbe();
}

void QuicConnectivityProbingManager::CancelProbe() {
  network_ = NetworkChangeNotifier::kInvalidNetworkHandle;
  peer_address_ = QuicSocketAddress();
  socket_.reset();
  writer_.reset();
  reader_.reset();
  retry_count_ = 0;
  initial_timeout_ms_ = 0;
  retransmit_timer_.Stop();
}

void QuicConnectivityProbingManager::StartProbeNetwork(
    NetworkChangeNotifier::NetworkHandle network,
    const QuicSocketAddress& peer_address,
    std::unique_ptr<DatagramClientSocket> socket,
    std::unique_ptr<QuicChromiumPacketWriter> writer,
    std::unique_ptr<QuicChromiumPacketReader> reader,
    base::TimeDelta initial_timeout) {
  // Start a new probe will always cancel the previous one.
  CancelProbe();

  network_ = network;
  peer_address_ = peer_address;
  socket_ = std::move(socket);
  writer_ = std::move(writer);
  reader_ = std::move(reader);
  initial_timeout_ms_ = initial_timeout.InMilliseconds();

  reader_->StartReading();
  SendConnectivityProbingPacket(initial_timeout);
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

  DVLOG(1) << "Current probing is live at ip:port: "
           << local_address.ToString();

  if (QuicSocketAddressImpl(local_address) != self_address.impl() &&
      peer_address_ != peer_address) {
    DVLOG(1) << "Probing response is ignored as probing was cancelled "
             << "or succeeded.";
    return;
  }

  // TODO(zhongyi): add metrics collection.
  // Notify the delegate that the probe succeeds and reset everything.
  delegate_->OnProbeNetworkSucceeded(network_, self_address, std::move(socket_),
                                     std::move(writer_), std::move(reader_));
  CancelProbe();
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
  int64_t timeout_ms = (2 << retry_count_) * initial_timeout_ms_;
  if (timeout_ms > kMaxProbingTimeoutMs) {
    delegate_->OnProbeNetworkFailed(network_);
    CancelProbe();
    return;
  }
  retry_count_++;

  SendConnectivityProbingPacket(base::TimeDelta::FromMilliseconds(timeout_ms));
}

}  // namespace net
