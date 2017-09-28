// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/chromium/quic_http_utils.h"

#include <utility>

#include "net/quic/platform/api/quic_endian.h"

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

QuicVersionVector FilterSupportedAltSvcVersions(
    const std::string& quic_protocol_id,
    const std::vector<uint32_t>& alt_svc_versions,
    const QuicVersionVector& supported_versions) {
  bool is_ietf_advertisement = quic_protocol_id == ("hq");
  DCHECK(is_ietf_advertisement || quic_protocol_id == "quic")
      << quic_protocol_id;

  QuicVersionVector supported_alt_svc_versions;
  for (uint32_t alt_svc_version : alt_svc_versions) {
    for (QuicVersion supported_version : supported_versions) {
      if (is_ietf_advertisement) {
        // Advertised version will be the 4-byte on the wire value.
        QuicVersionLabel version_label = alt_svc_version;
        if (!FLAGS_quic_reloadable_flag_quic_use_net_byte_order_version_label) {
          version_label = QuicEndian::NetToHost32(version_label);
        }
        if (version_label == QuicVersionToQuicVersionLabel(supported_version)) {
          supported_alt_svc_versions.push_back(supported_version);
          break;
        }
      } else {
        // Advertised version is the value of the QuicVersion enum.
        if (alt_svc_version == supported_version) {
          supported_alt_svc_versions.push_back(supported_version);
          break;
        }
      }
    }
  }
  return supported_alt_svc_versions;
}

}  // namespace net
