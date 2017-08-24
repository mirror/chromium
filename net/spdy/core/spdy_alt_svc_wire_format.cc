// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/core/spdy_alt_svc_wire_format.h"

#include <algorithm>
#include <cctype>
#include <limits>

#include "base/logging.h"
#include "net/spdy/platform/api/spdy_string_utils.h"

namespace net {

namespace {

template <class T>
bool ParsePositiveIntegerImpl(SpdyStringPiece::const_iterator c,
                              SpdyStringPiece::const_iterator end,
                              T* value) {
  *value = 0;
  for (; c != end && std::isdigit(*c); ++c) {
    if (*value > std::numeric_limits<T>::max() / 10) {
      return false;
    }
    *value *= 10;
    if (*value > std::numeric_limits<T>::max() - (*c - '0')) {
      return false;
    }
    *value += *c - '0';
  }
  return (c == end && *value > 0);
}

const char VALUE_TO_HEX_DIGIT[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

// Will not include leading zeros. Example:
// Input:   0xCDAB2301
// Output:  "123ABCD"
SpdyString LittleEndianUIntToHexString(uint32_t input) {
  char hex_digits[8];
  for (int i = 0; i < 8; i += 2) {
    uint32_t lower_nibble = input & 0xF;
    input >>= 4;
    uint32_t upper_nibble = input & 0xF;
    input >>= 4;
    DCHECK_LT(lower_nibble, 16u);
    DCHECK_LT(upper_nibble, 16u);
    hex_digits[i] = VALUE_TO_HEX_DIGIT[upper_nibble];
    hex_digits[i + 1] = VALUE_TO_HEX_DIGIT[lower_nibble];
  }
  // Trim leading zeros except for last digit.
  int i = 0;
  for (; hex_digits[i] == '0' && i < 7; ++i)
    ;
  return SpdyString(&(hex_digits[i]), 8 - i);
}

const char HEX_DIGIT_TO_VALUE[256] = {
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    -128, -128,
    -128, -128, -128, -128, -128, 10,   11,   12,   13,   14,   15,   -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, 10,   11,   12,   13,   14,   15,   -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128,
    -128, -128, -128, -128};

// Input must be 1 to 8 characters long. Example:
// Input:  "123ABCD"
// Output: 0xCDAB2301
bool LittleEndianUIntFromHexString(const std::string& hex_string,
                                   uint32_t* output) {
  DCHECK(!hex_string.empty());
  DCHECK_LE(hex_string.size(), 8u);
  *output = 0;
  int i = hex_string.size() - 1;
  while (i >= 1) {
    int upper_nibble = HEX_DIGIT_TO_VALUE[(int)hex_string[i - 1]];
    int lower_nibble = HEX_DIGIT_TO_VALUE[(int)hex_string[i]];
    int byte_value = upper_nibble << 4 | lower_nibble;
    if (byte_value < 0) {
      return false;
    }
    *output = (*output << 8) | byte_value;
    i -= 2;
  }
  if (i == 0) {
    int lower_nibble = HEX_DIGIT_TO_VALUE[(int)hex_string[i]];
    if (lower_nibble < 0) {
      return false;
    }
    *output = (*output << 8) | lower_nibble;
  }
  return true;
}

}  // namespace

SpdyAltSvcWireFormat::AlternativeService::AlternativeService() {}

SpdyAltSvcWireFormat::AlternativeService::AlternativeService(
    const SpdyString& protocol_id,
    const SpdyString& host,
    uint16_t port,
    uint32_t max_age,
    VersionVector version)
    : protocol_id(protocol_id),
      host(host),
      port(port),
      max_age(max_age),
      version(version) {}

SpdyAltSvcWireFormat::AlternativeService::~AlternativeService() {}

SpdyAltSvcWireFormat::AlternativeService::AlternativeService(
    const AlternativeService& other) = default;

// static
bool SpdyAltSvcWireFormat::ParseHeaderFieldValue(
    SpdyStringPiece value,
    AlternativeServiceVector* altsvc_vector) {
  // Empty value is invalid according to the specification.
  if (value.empty()) {
    return false;
  }
  altsvc_vector->clear();
  if (value == SpdyStringPiece("clear")) {
    return true;
  }
  SpdyStringPiece::const_iterator c = value.begin();
  while (c != value.end()) {
    // Parse protocol-id.
    SpdyStringPiece::const_iterator percent_encoded_protocol_id_end =
        std::find(c, value.end(), '=');
    SpdyString protocol_id;
    if (percent_encoded_protocol_id_end == c ||
        !PercentDecode(c, percent_encoded_protocol_id_end, &protocol_id)) {
      return false;
    }
    // Check for IETF format for advertising QUIC:
    // h2q=":49288";quic=51303338;quic=51303334
    const bool is_ietf_format_quic = (protocol_id.compare("h2q") == 0);
    c = percent_encoded_protocol_id_end;
    if (c == value.end()) {
      return false;
    }
    // Parse alt-authority.
    DCHECK_EQ('=', *c);
    ++c;
    if (c == value.end() || *c != '"') {
      return false;
    }
    ++c;
    SpdyStringPiece::const_iterator alt_authority_begin = c;
    for (; c != value.end() && *c != '"'; ++c) {
      // Decode backslash encoding.
      if (*c != '\\') {
        continue;
      }
      ++c;
      if (c == value.end()) {
        return false;
      }
    }
    if (c == alt_authority_begin || c == value.end()) {
      return false;
    }
    DCHECK_EQ('"', *c);
    SpdyString host;
    uint16_t port;
    if (!ParseAltAuthority(alt_authority_begin, c, &host, &port)) {
      return false;
    }
    ++c;
    // Parse parameters.
    uint32_t max_age = 86400;
    VersionVector version;
    SpdyStringPiece::const_iterator parameters_end =
        std::find(c, value.end(), ',');
    while (c != parameters_end) {
      SkipWhiteSpace(&c, parameters_end);
      if (c == parameters_end) {
        break;
      }
      if (*c != ';') {
        return false;
      }
      ++c;
      SkipWhiteSpace(&c, parameters_end);
      if (c == parameters_end) {
        break;
      }
      SpdyString parameter_name;
      for (; c != parameters_end && *c != '=' && *c != ' ' && *c != '\t'; ++c) {
        parameter_name.push_back(tolower(*c));
      }
      SkipWhiteSpace(&c, parameters_end);
      if (c == parameters_end || *c != '=') {
        return false;
      }
      ++c;
      SkipWhiteSpace(&c, parameters_end);
      SpdyStringPiece::const_iterator parameter_value_begin = c;
      for (; c != parameters_end && *c != ';' && *c != ' ' && *c != '\t'; ++c) {
      }
      if (c == parameter_value_begin) {
        return false;
      }
      if (parameter_name.compare("ma") == 0) {
        if (!ParsePositiveInteger32(parameter_value_begin, c, &max_age)) {
          return false;
        }
      } else if (!is_ietf_format_quic && parameter_name.compare("v") == 0) {
        // Version is a comma separated list of positive integers enclosed in
        // quotation marks.  Since it can contain commas, which are not
        // delineating alternative service entries, |parameters_end| and |c| can
        // be invalid.
        if (*parameter_value_begin != '"') {
          return false;
        }
        c = std::find(parameter_value_begin + 1, value.end(), '"');
        if (c == value.end()) {
          return false;
        }
        ++c;
        parameters_end = std::find(c, value.end(), ',');
        SpdyStringPiece::const_iterator v_begin = parameter_value_begin + 1;
        while (v_begin < c) {
          SpdyStringPiece::const_iterator v_end = v_begin;
          while (v_end < c - 1 && *v_end != ',') {
            ++v_end;
          }
          uint16_t v;
          if (!ParsePositiveInteger16(v_begin, v_end, &v)) {
            return false;
          }
          version.push_back(v);
          v_begin = v_end + 1;
          if (v_begin == c - 1) {
            // List ends in comma.
            return false;
          }
        }
      } else if (is_ietf_format_quic && parameter_name.compare("quic") == 0) {
        // IETF format for advertising QUIC. Version is hex encoding of QUIC
        // version tag. Hex-encoded string should not include leading "0x" or
        // leading zeros, unless the version is just "0".
        // Example for advertising QUIC versions "Q038" and "Q034":
        // h2q=":49288";quic=51303338;quic=51303334
        if (*parameter_value_begin == '0' && c != parameter_value_begin + 1) {
          return false;
        }
        // Versions will be stored as the little-endian uint32_t value of the
        // version tag. Example: QUIC version "Q038", which is advertised as:
        // h2q=":49288";quic=51303338
        // ... will be stored in |versions| as 0x38333051.
        uint32_t quic_version_tag;
        if (!LittleEndianUIntFromHexString(
                std::string(parameter_value_begin, c), &quic_version_tag)) {
          return false;
        }
        version.push_back(quic_version_tag);
        /*uint32_t quic_version_tag_reversed;
        if (!base::HexStringToUInt(std::string(parameter_value_begin, c),
                                   &quic_version_tag_reversed)) {
          return false;
        }
        version.push_back(ReverseUIntByteOrder(quic_version_tag_reversed));*/
      }
    }
    altsvc_vector->emplace_back(protocol_id, host, port, max_age, version);
    for (; c != value.end() && (*c == ' ' || *c == '\t' || *c == ','); ++c) {
    }
  }
  return true;
}

// static
SpdyString SpdyAltSvcWireFormat::SerializeHeaderFieldValue(
    const AlternativeServiceVector& altsvc_vector) {
  if (altsvc_vector.empty()) {
    return SpdyString("clear");
  }
  const char kNibbleToHex[] = "0123456789ABCDEF";
  SpdyString value;
  for (const AlternativeService& altsvc : altsvc_vector) {
    if (!value.empty()) {
      value.push_back(',');
    }
    // Check for IETF format for advertising QUIC.
    const bool is_ietf_format_quic = (altsvc.protocol_id.compare("h2q") == 0);
    // Percent escape protocol id according to
    // http://tools.ietf.org/html/rfc7230#section-3.2.6.
    for (char c : altsvc.protocol_id) {
      if (isalnum(c)) {
        value.push_back(c);
        continue;
      }
      switch (c) {
        case '!':
        case '#':
        case '$':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
          value.push_back(c);
          break;
        default:
          value.push_back('%');
          // Network byte order is big-endian.
          value.push_back(kNibbleToHex[c >> 4]);
          value.push_back(kNibbleToHex[c & 0x0f]);
          break;
      }
    }
    value.push_back('=');
    value.push_back('"');
    for (char c : altsvc.host) {
      if (c == '"' || c == '\\') {
        value.push_back('\\');
      }
      value.push_back(c);
    }
    SpdyStringAppendF(&value, ":%d\"", altsvc.port);
    if (altsvc.max_age != 86400) {
      SpdyStringAppendF(&value, "; ma=%d", altsvc.max_age);
    }
    if (!altsvc.version.empty()) {
      if (is_ietf_format_quic) {
        for (VersionVector::const_iterator it = altsvc.version.begin();
             it != altsvc.version.end(); ++it) {
          value.append("; quic=");
          uint32_t quic_version_tag = *it;
          value.append(LittleEndianUIntToHexString(quic_version_tag));
        }
      } else {
        value.append("; v=\"");
        for (VersionVector::const_iterator it = altsvc.version.begin();
             it != altsvc.version.end(); ++it) {
          if (it != altsvc.version.begin()) {
            value.append(",");
          }
          SpdyStringAppendF(&value, "%d", *it);
        }
        value.append("\"");
      }
    }
  }
  return value;
}

// static
void SpdyAltSvcWireFormat::SkipWhiteSpace(SpdyStringPiece::const_iterator* c,
                                          SpdyStringPiece::const_iterator end) {
  for (; *c != end && (**c == ' ' || **c == '\t'); ++*c) {
  }
}

// static
bool SpdyAltSvcWireFormat::PercentDecode(SpdyStringPiece::const_iterator c,
                                         SpdyStringPiece::const_iterator end,
                                         SpdyString* output) {
  output->clear();
  for (; c != end; ++c) {
    if (*c != '%') {
      output->push_back(*c);
      continue;
    }
    DCHECK_EQ('%', *c);
    ++c;
    if (c == end || !std::isxdigit(*c)) {
      return false;
    }
    // Network byte order is big-endian.
    char decoded = SpdyHexDigitToInt(*c) << 4;
    ++c;
    if (c == end || !std::isxdigit(*c)) {
      return false;
    }
    decoded += SpdyHexDigitToInt(*c);
    output->push_back(decoded);
  }
  return true;
}

// static
bool SpdyAltSvcWireFormat::ParseAltAuthority(
    SpdyStringPiece::const_iterator c,
    SpdyStringPiece::const_iterator end,
    SpdyString* host,
    uint16_t* port) {
  host->clear();
  if (c == end) {
    return false;
  }
  if (*c == '[') {
    for (; c != end && *c != ']'; ++c) {
      if (*c == '"') {
        // Port is mandatory.
        return false;
      }
      host->push_back(*c);
    }
    if (c == end) {
      return false;
    }
    DCHECK_EQ(']', *c);
    host->push_back(*c);
    ++c;
  } else {
    for (; c != end && *c != ':'; ++c) {
      if (*c == '"') {
        // Port is mandatory.
        return false;
      }
      if (*c == '\\') {
        ++c;
        if (c == end) {
          return false;
        }
      }
      host->push_back(*c);
    }
  }
  if (c == end || *c != ':') {
    return false;
  }
  DCHECK_EQ(':', *c);
  ++c;
  return ParsePositiveInteger16(c, end, port);
}

// static
bool SpdyAltSvcWireFormat::ParsePositiveInteger16(
    SpdyStringPiece::const_iterator c,
    SpdyStringPiece::const_iterator end,
    uint16_t* value) {
  return ParsePositiveIntegerImpl<uint16_t>(c, end, value);
}

// static
bool SpdyAltSvcWireFormat::ParsePositiveInteger32(
    SpdyStringPiece::const_iterator c,
    SpdyStringPiece::const_iterator end,
    uint32_t* value) {
  return ParsePositiveIntegerImpl<uint32_t>(c, end, value);
}

}  // namespace net
