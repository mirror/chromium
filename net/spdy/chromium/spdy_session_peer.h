// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_CHROMIUM_SPDY_SESSION_PEER_H_
#define NET_SPDY_CHROMIUM_SPDY_SESSION_PEER_H_

#include <cstddef>
#include <cstdint>

#include "net/spdy/chromium/spdy_session.h"
#include "url/gurl.h"

namespace net {

namespace test {

class SpdySessionPeer {
 public:
  // Private methods.

  static void MaybeSendPrefacePing(SpdySession* session) {
    session->MaybeSendPrefacePing();
  }

  static void WritePingFrame(SpdySession* session,
                             SpdyPingId unique_id,
                             bool is_ack) {
    session->WritePingFrame(unique_id, is_ack);
  }

  static void CheckPingStatus(SpdySession* session,
                              base::TimeTicks last_check_time) {
    session->CheckPingStatus(last_check_time);
  }

  static void RecordPushedStreamVaryResponseHeaderHistogram(
      const SpdyHeaderBlock& headers) {
    SpdySession::RecordPushedStreamVaryResponseHeaderHistogram(headers);
  }

  static bool OnUnknownFrame(SpdySession* session,
                             SpdyStreamId stream_id,
                             uint8_t frame_type) {
    return session->OnUnknownFrame(stream_id, frame_type);
  }

  static void IncreaseSendWindowSize(SpdySession* session,
                                     int delta_window_size) {
    session->IncreaseSendWindowSize(delta_window_size);
  }

  static void DecreaseSendWindowSize(SpdySession* session,
                                     int32_t delta_window_size) {
    session->DecreaseSendWindowSize(delta_window_size);
  }

  static void IncreaseRecvWindowSize(SpdySession* session,
                                     int32_t delta_window_size) {
    session->IncreaseRecvWindowSize(delta_window_size);
  }

  static void DecreaseRecvWindowSize(SpdySession* session,
                                     int32_t delta_window_size) {
    session->DecreaseRecvWindowSize(delta_window_size);
  }

  // Accessors for private members.

  static void set_in_io_loop(SpdySession* session, bool in_io_loop) {
    session->in_io_loop_ = in_io_loop;
  }

  static void set_stream_hi_water_mark(SpdySession* session,
                                       SpdyStreamId stream_hi_water_mark) {
    session->stream_hi_water_mark_ = stream_hi_water_mark;
  }

  static void set_last_accepted_push_stream_id(
      SpdySession* session,
      SpdyStreamId last_accepted_push_stream_id) {
    session->last_accepted_push_stream_id_ = last_accepted_push_stream_id;
  }

  static size_t num_pushed_streams(SpdySession* session) {
    return session->num_pushed_streams_;
  }

  static size_t num_active_pushed_streams(SpdySession* session) {
    return session->num_active_pushed_streams_;
  }

  static size_t max_concurrent_streams(SpdySession* session) {
    return session->max_concurrent_streams_;
  }

  static void set_max_concurrent_streams(SpdySession* session,
                                         size_t max_concurrent_streams) {
    session->max_concurrent_streams_ = max_concurrent_streams;
  }

  static void set_max_concurrent_pushed_streams(
      SpdySession* session,
      size_t max_concurrent_pushed_streams) {
    session->max_concurrent_pushed_streams_ = max_concurrent_pushed_streams;
  }

  static int64_t pings_in_flight(SpdySession* session) {
    return session->pings_in_flight_;
  }

  static SpdyPingId next_ping_id(SpdySession* session) {
    return session->next_ping_id_;
  }

  static base::TimeTicks last_read_time(SpdySession* session) {
    return session->last_read_time_;
  }

  static void set_last_read_time(SpdySession* session,
                                 base::TimeTicks last_read_time) {
    session->last_read_time_ = last_read_time;
  }

  static bool check_ping_status_pending(SpdySession* session) {
    return session->check_ping_status_pending_;
  }

  static void set_check_ping_status_pending(SpdySession* session,
                                            bool check_ping_status_pending) {
    session->check_ping_status_pending_ = check_ping_status_pending;
  }

  static int32_t session_send_window_size(SpdySession* session) {
    return session->session_send_window_size_;
  }

  static int32_t session_recv_window_size(SpdySession* session) {
    return session->session_recv_window_size_;
  }

  static void set_session_recv_window_size(SpdySession* session,
                                           int32_t session_recv_window_size) {
    session->session_recv_window_size_ = session_recv_window_size;
  }

  static int32_t session_unacked_recv_window_bytes(SpdySession* session) {
    return session->session_unacked_recv_window_bytes_;
  }

  static int32_t stream_initial_send_window_size(SpdySession* session) {
    return session->stream_initial_send_window_size_;
  }

  static void set_connection_at_risk_of_loss_time(SpdySession* session,
                                                  base::TimeDelta duration) {
    session->connection_at_risk_of_loss_time_ = duration;
  }

  static void set_hung_interval(SpdySession* session,
                                base::TimeDelta duration) {
    session->hung_interval_ = duration;
  }

  // Quantities derived from private members.

  static size_t num_active_streams(SpdySession* session) {
    return session->active_streams_.size();
  }

  static size_t num_created_streams(SpdySession* session) {
    return session->created_streams_.size();
  }

  static size_t pending_create_stream_queue_size(SpdySession* session,
                                                 RequestPriority priority) {
    DCHECK_GE(priority, MINIMUM_PRIORITY);
    DCHECK_LE(priority, MAXIMUM_PRIORITY);
    return session->pending_create_stream_queues_[priority].size();
  }

  static size_t num_unclaimed_pushed_streams(SpdySession* session) {
    return session->unclaimed_pushed_streams_.size();
  }

  static bool has_unclaimed_pushed_stream_for_url(SpdySession* session,
                                                  const GURL& url) {
    return session->unclaimed_pushed_streams_.count(url) > 0;
  }
};

}  // namespace test

}  // namespace net

#endif  // NET_SPDY_CHROMIUM_SPDY_SESSION_PEER_H_
