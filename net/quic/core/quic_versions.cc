// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/quic_versions.h"

#include "net/quic/core/quic_error_codes.h"
#include "net/quic/core/quic_tag.h"
#include "net/quic/core/quic_types.h"
#include "net/quic/platform/api/quic_bug_tracker.h"
#include "net/quic/platform/api/quic_endian.h"
#include "net/quic/platform/api/quic_flag_utils.h"
#include "net/quic/platform/api/quic_flags.h"
#include "net/quic/platform/api/quic_logging.h"

using std::string;

namespace net {
namespace {

// Constructs a version label from the 4 bytes such that the on-the-wire
// order will be: d, c, b, a.
QuicVersionLabel MakeVersionLabel(char a, char b, char c, char d) {
  if (!FLAGS_quic_reloadable_flag_quic_use_net_byte_order_version_label) {
    return MakeQuicTag(a, b, c, d);
  }
  QUIC_FLAG_COUNT_N(quic_reloadable_flag_quic_use_net_byte_order_version_label,
                    1, 10);
  return MakeQuicTag(d, c, b, a);
}

// Constructs a QuicVersionLabel from the provided QuicVersion and
// HandshakeProtocol.
QuicVersionLabel LabelFromVersionAndProtocol(QuicVersion version,
                                             HandshakeProtocol handshake) {
  char proto = 0;
  switch (handshake) {
    case PROTOCOL_QUIC_CRYPTO:
      proto = 'Q';
      break;
    case PROTOCOL_TLS1_3:
      if (!FLAGS_quic_supports_tls_handshake) {
        QUIC_BUG << "TLS use attempted when not enabled";
      }
      proto = 'T';
      break;
    default:
      QUIC_LOG(ERROR) << "Invalid HandshakeProtocol: " << handshake;
      return 0;
  }
  switch (version) {
    case QUIC_VERSION_35:
      return MakeVersionLabel(proto, '0', '3', '5');
    case QUIC_VERSION_37:
      return MakeVersionLabel(proto, '0', '3', '7');
    case QUIC_VERSION_38:
      return MakeVersionLabel(proto, '0', '3', '8');
    case QUIC_VERSION_39:
      return MakeVersionLabel(proto, '0', '3', '9');
    case QUIC_VERSION_41:
      return MakeVersionLabel(proto, '0', '4', '1');
    case QUIC_VERSION_42:
      return MakeVersionLabel(proto, '0', '4', '2');
    default:
      // This shold be an ERROR because we should never attempt to convert an
      // invalid QuicVersion to be written to the wire.
      QUIC_LOG(ERROR) << "Unsupported QuicVersion: " << version;
      return 0;
  }
}

std::pair<QuicVersion, HandshakeProtocol> VersionAndProtocolFromLabel(
    QuicVersionLabel version_label) {
  std::vector<HandshakeProtocol> protocols = {PROTOCOL_QUIC_CRYPTO};
  if (FLAGS_quic_supports_tls_handshake) {
    protocols.push_back(PROTOCOL_TLS1_3);
  }
  for (QuicVersion version : kSupportedQuicVersions) {
    for (HandshakeProtocol handshake : protocols) {
      if (version_label == LabelFromVersionAndProtocol(version, handshake)) {
        return std::make_pair(version, handshake);
      }
    }
  }
  // Reading from the client so this should not be considered an ERROR.
  QUIC_DLOG(INFO) << "Unsupported QuicVersionLabel version: "
                  << QuicVersionLabelToString(version_label);
  return std::make_pair(QUIC_VERSION_UNSUPPORTED, PROTOCOL_UNSUPPORTED);
}

}  // namespace

QuicVersionVector AllSupportedVersions() {
  QuicVersionVector supported_versions;
  for (size_t i = 0; i < arraysize(kSupportedQuicVersions); ++i) {
    supported_versions.push_back(kSupportedQuicVersions[i]);
  }
  return supported_versions;
}

QuicVersionVector CurrentSupportedVersions() {
  return FilterSupportedVersions(AllSupportedVersions());
}

QuicVersionVector FilterSupportedVersions(QuicVersionVector versions) {
  QuicVersionVector filtered_versions(versions.size());
  filtered_versions.clear();  // Guaranteed by spec not to change capacity.
  for (QuicVersion version : versions) {
    if (version == QUIC_VERSION_42) {
      if (GetQuicFlag(FLAGS_quic_enable_version_42) &&
          FLAGS_quic_reloadable_flag_quic_enable_version_41 &&
          FLAGS_quic_reloadable_flag_quic_enable_version_39 &&
          FLAGS_quic_reloadable_flag_quic_enable_version_38) {
        filtered_versions.push_back(version);
      }
    } else if (version == QUIC_VERSION_41) {
      if (FLAGS_quic_reloadable_flag_quic_enable_version_41 &&
          FLAGS_quic_reloadable_flag_quic_enable_version_39 &&
          FLAGS_quic_reloadable_flag_quic_enable_version_38) {
        filtered_versions.push_back(version);
      }
    } else if (version == QUIC_VERSION_39) {
      if (FLAGS_quic_reloadable_flag_quic_enable_version_39 &&
          FLAGS_quic_reloadable_flag_quic_enable_version_38) {
        filtered_versions.push_back(version);
      }
    } else if (version == QUIC_VERSION_38) {
      if (FLAGS_quic_reloadable_flag_quic_enable_version_38) {
        filtered_versions.push_back(version);
      }
    } else {
      filtered_versions.push_back(version);
    }
  }
  return filtered_versions;
}

QuicVersionVector VersionOfIndex(const QuicVersionVector& versions, int index) {
  QuicVersionVector version;
  int version_count = versions.size();
  if (index >= 0 && index < version_count) {
    version.push_back(versions[index]);
  } else {
    version.push_back(QUIC_VERSION_UNSUPPORTED);
  }
  return version;
}

QuicVersionLabel QuicVersionToQuicVersionLabel(const QuicVersion version) {
  return LabelFromVersionAndProtocol(version, PROTOCOL_QUIC_CRYPTO);
}

string QuicVersionLabelToString(QuicVersionLabel version_label) {
  if (!FLAGS_quic_reloadable_flag_quic_use_net_byte_order_version_label) {
    return QuicTagToString(version_label);
  }
  QUIC_FLAG_COUNT_N(quic_reloadable_flag_quic_use_net_byte_order_version_label,
                    2, 10);
  return QuicTagToString(QuicEndian::HostToNet32(version_label));
}

QuicVersion QuicVersionLabelToQuicVersion(QuicVersionLabel version_label) {
  return VersionAndProtocolFromLabel(version_label).first;
}

HandshakeProtocol QuicVersionLabelToHandshakeProtocol(
    QuicVersionLabel version_label) {
  return VersionAndProtocolFromLabel(version_label).second;
}

#define RETURN_STRING_LITERAL(x) \
  case x:                        \
    return #x

string QuicVersionToString(const QuicVersion version) {
  switch (version) {
    RETURN_STRING_LITERAL(QUIC_VERSION_35);
    RETURN_STRING_LITERAL(QUIC_VERSION_37);
    RETURN_STRING_LITERAL(QUIC_VERSION_38);
    RETURN_STRING_LITERAL(QUIC_VERSION_39);
    RETURN_STRING_LITERAL(QUIC_VERSION_41);
    RETURN_STRING_LITERAL(QUIC_VERSION_42);
    default:
      return "QUIC_VERSION_UNSUPPORTED";
  }
}

string QuicVersionVectorToString(const QuicVersionVector& versions) {
  string result = "";
  for (size_t i = 0; i < versions.size(); ++i) {
    if (i != 0) {
      result.append(",");
    }
    result.append(QuicVersionToString(versions[i]));
  }
  return result;
}

}  // namespace net
