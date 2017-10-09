// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/chromium/quic_http_utils.h"

#include <utility>

namespace net {

SpdyPriority ConvertRequestPriorityToQuicPriority(
    const RequestPriority priority) {
  DCHECK_GE(priority, MINIMUM_PRIORITY);
  DCHECK_LE(priority, MAXIMUM_PRIORITY);
  return static_cast<SpdyPriority>(HIGHEST - priority);
}

NET_EXPORT_PRIVATE RequestPriority
ConvertQuicPriorityToRequestPriority(SpdyPriority priority) {
  // Handle invalid values gracefully.
  return (priority >= 5) ? IDLE
                         : static_cast<RequestPriority>(HIGHEST - priority);
}

std::unique_ptr<base::Value> QuicRequestNetLogCallback(
    QuicStreamId stream_id,
    const SpdyHeaderBlock* headers,
    SpdyPriority priority,
    NetLogCaptureMode capture_mode) {
  std::unique_ptr<base::DictionaryValue> dict(
      static_cast<base::DictionaryValue*>(
          SpdyHeaderBlockNetLogCallback(headers, capture_mode).release()));
  dict->SetInteger("quic_priority", static_cast<int>(priority));
  dict->SetInteger("quic_stream_id", static_cast<int>(stream_id));
  return std::move(dict);
}

QuicTransportVersionVector FilterSupportedAltSvcVersions(
    const std::string& quic_protocol_id,
    const std::vector<uint32_t>& alt_svc_versions,
    const QuicTransportVersionVector& supported_versions) {
  QuicTransportVersionVector supported_alt_svc_versions;
  if (quic_protocol_id.compare("hq") == 0) {
    // Using IETF format for advertising QUIC. In this case,
    // |alternative_service_entry.version| will store QUIC version tags.
    for (uint32_t quic_version_label : alt_svc_versions) {
      for (QuicTransportVersion supported : supported_versions) {
        if (QuicVersionToQuicVersionLabel(supported) == quic_version_label)
          supported_alt_svc_versions.push_back(supported);
      }
    }
  } else if (quic_protocol_id.compare("quic") == 0) {
    for (uint32_t quic_version : alt_svc_versions) {
      for (QuicTransportVersion supported : supported_versions) {
        if (supported == quic_version)
          supported_alt_svc_versions.push_back(supported);
      }
    }
  }
  return supported_alt_svc_versions;
}

}  // namespace net
