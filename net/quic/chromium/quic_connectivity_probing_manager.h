// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CHROMIUM_QUIC_CONNECTIVITY_PROBING_MANAGER_H_
#define NET_QUIC_CHROMIUM_QUIC_CONNECTIVITY_PROBING_MANAGER_H_

// #include "base/macros.h"
#include "base/time/time.h"
// #include "net/base/completion_callback.h"
#include "net/base/load_timing_info.h"
#include "net/base/net_error_details.h"
#include "net/base/net_export.h"
#include "net/log/net_log_with_source.h"
#include "net/quic/chromium/quic_chromium_client_session.h"
#include "net/quic/chromium/quic_chromium_packet_reader.h"
#include "net/quic/chromium/quic_chromium_packet_writer.h"
#include "net/quic/core/quic_packets.h"
#include "net/quic/core/quic_time.h"

namespace net {

// Responsible for sending and retransmitting connectivity probing packet on a
// designated path to the specified peer, and for notifying associated session
// when connectivity probe fails or succeeds.
class QuicConnectivityProbingManager {
 public:
  QuicConnectivityProbingManager(QuicChromiumClientSession* session);
  ~QuicConnectivityProbingManager();

  void CancelProbe();

  void StartProbeNetwork(NetworkChangeNotifier::NetworkHandle network,
                         const QuicSocketAddress& peer_address,
                         std::unique_ptr<DatagramClientSocket> socket,
                         std::unique_ptr<QuicChromiumPacketWriter> writer,
                         std::unique_ptr<QuicChromiumPacketReader> reader,
                         int initial_timeout);

  void OnConnectivityProbingReceived(const QuicSocketAddress& self_address,
                                     const QuicSocketAddress& peer_address);

 private:
  void SendConnectivityProbingPacket(int timeout);

  void MaybeResendConnectivityProbingPacket();
  QuicChromiumClientSession* session_;

  NetworkChangeNotifier::NetworkHandle network_;
  QuicSocketAddress peer_address_;

  std::unique_ptr<DatagramClientSocket> socket_;
  std::unique_ptr<QuicChromiumPacketWriter> writer_;
  std::unique_ptr<QuicChromiumPacketReader> reader_;

  int retry_count_;
  int initial_timeout_;
  base::OneShotTimer retransmit_timer_;

  base::WeakPtrFactory<QuicConnectivityProbingManager> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(QuicConnectivityProbingManager);
};

}  // namespace net

#endif  // NET_QUIC_CHROMIUM_QUIC_CONNECTIVITY_PROBING_MANAGER_H_
