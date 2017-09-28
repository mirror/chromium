// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CHROMIUM_QUIC_HTTP_UTILS_H_
#define NET_QUIC_CHROMIUM_QUIC_HTTP_UTILS_H_

#include "base/values.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"
#include "net/quic/core/quic_packets.h"
#include "net/spdy/core/spdy_header_block.h"
#include "net/spdy/core/spdy_protocol.h"

namespace net {

NET_EXPORT_PRIVATE SpdyPriority
ConvertRequestPriorityToQuicPriority(RequestPriority priority);

NET_EXPORT_PRIVATE RequestPriority
ConvertQuicPriorityToRequestPriority(SpdyPriority priority);

// Converts a SpdyHeaderBlock and priority into NetLog event parameters.
NET_EXPORT std::unique_ptr<base::Value> QuicRequestNetLogCallback(
    QuicStreamId stream_id,
    const SpdyHeaderBlock* headers,
    SpdyPriority priority,
    NetLogCaptureMode capture_mode);

// Returns the list of supported QUIC versions in the Alt-Svc advertisment
// specified by |quic_protocol_id| and |alt_svc_versions|.
NET_EXPORT_PRIVATE QuicVersionVector
FilterSupportedAltSvcVersions(const std::string& quic_protocol_id,
                              const std::vector<uint32_t>& alt_svc_versions,
                              const QuicVersionVector& supported_versions);

}  // namespace net

#endif  // NET_QUIC_CHROMIUM_QUIC_HTTP_UTILS_H_
