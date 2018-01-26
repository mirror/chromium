// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/spdy_utils.h"

#include <memory>
#include <vector>

#include "net/quic/platform/api/quic_flag_utils.h"
#include "net/quic/platform/api/quic_flags.h"
#include "net/quic/platform/api/quic_logging.h"
#include "net/quic/platform/api/quic_map_util.h"
#include "net/quic/platform/api/quic_string_piece.h"
#include "net/quic/platform/api/quic_text_utils.h"
#include "net/quic/platform/api/quic_url_utils.h"
#include "net/spdy/core/spdy_frame_builder.h"
#include "net/spdy/core/spdy_framer.h"
#include "net/spdy/core/spdy_protocol.h"

using std::string;

namespace net {

// static
bool SpdyUtils::ExtractContentLengthFromHeaders(int64_t* content_length,
                                                SpdyHeaderBlock* headers) {
  auto it = headers->find("content-length");
  if (it == headers->end()) {
    return false;
  } else {
    // Check whether multiple values are consistent.
    QuicStringPiece content_length_header = it->second;
    std::vector<QuicStringPiece> values =
        QuicTextUtils::Split(content_length_header, '\0');
    for (const QuicStringPiece& value : values) {
      uint64_t new_value;
      if (!QuicTextUtils::StringToUint64(value, &new_value)) {
        QUIC_DLOG(ERROR)
            << "Content length was either unparseable or negative.";
        return false;
      }
      if (*content_length < 0) {
        *content_length = new_value;
        continue;
      }
      if (new_value != static_cast<uint64_t>(*content_length)) {
        QUIC_DLOG(ERROR)
            << "Parsed content length " << new_value << " is "
            << "inconsistent with previously detected content length "
            << *content_length;
        return false;
      }
    }
    return true;
  }
}

bool SpdyUtils::CopyAndValidateHeaders(const QuicHeaderList& header_list,
                                       int64_t* content_length,
                                       SpdyHeaderBlock* headers) {
  for (const auto& p : header_list) {
    const string& name = p.first;
    if (name.empty()) {
      QUIC_DLOG(ERROR) << "Header name must not be empty.";
      return false;
    }

    if (QuicTextUtils::ContainsUpperCase(name)) {
      QUIC_DLOG(ERROR) << "Malformed header: Header name " << name
                       << " contains upper-case characters.";
      return false;
    }

    headers->AppendValueOrAddHeader(name, p.second);
  }

  if (QuicContainsKey(*headers, "content-length") &&
      !ExtractContentLengthFromHeaders(content_length, headers)) {
    return false;
  }

  QUIC_DVLOG(1) << "Successfully parsed headers: " << headers->DebugString();
  return true;
}

bool SpdyUtils::CopyAndValidateTrailers(const QuicHeaderList& header_list,
                                        size_t* final_byte_offset,
                                        SpdyHeaderBlock* trailers) {
  bool found_final_byte_offset = false;
  for (const auto& p : header_list) {
    const string& name = p.first;

    // Pull out the final offset pseudo header which indicates the number of
    // response body bytes expected.
    if (!found_final_byte_offset && name == kFinalOffsetHeaderKey &&
        QuicTextUtils::StringToSizeT(p.second, final_byte_offset)) {
      found_final_byte_offset = true;
      continue;
    }

    if (name.empty() || name[0] == ':') {
      QUIC_DLOG(ERROR)
          << "Trailers must not be empty, and must not contain pseudo-"
          << "headers. Found: '" << name << "'";
      return false;
    }

    if (QuicTextUtils::ContainsUpperCase(name)) {
      QUIC_DLOG(ERROR) << "Malformed header: Header name " << name
                       << " contains upper-case characters.";
      return false;
    }

    trailers->AppendValueOrAddHeader(name, p.second);
  }

  if (!found_final_byte_offset) {
    QUIC_DLOG(ERROR) << "Required key '" << kFinalOffsetHeaderKey
                     << "' not present";
    return false;
  }

  // TODO(rjshade): Check for other forbidden keys, following the HTTP/2 spec.

  QUIC_DVLOG(1) << "Successfully parsed Trailers: " << trailers->DebugString();
  return true;
}

// static
string SpdyUtils::GetPromisedUrlFromHeaderBlock(
    const SpdyHeaderBlock& headers) {
  // RFC 7540, Section 8.1.2.3:
  // All HTTP/2 requests MUST include exactly one valid value for the ":method",
  // ":scheme", and ":path" pseudo-header fields, unless it is a CONNECT
  // request.

  // RFC 7540, Section  8.2.1:
  // The header fields in PUSH_PROMISE and any subsequent CONTINUATION frames
  // MUST be a valid and complete set of request header fields (Section
  // 8.1.2.3).  The server MUST include a method in the ":method" pseudo-header
  // field that is safe and cacheable.
  //
  // RFC 7231, Section  4.2.1:
  // Of the request methods defined by this specification, the GET, HEAD,
  // OPTIONS, and TRACE methods are defined to be safe.
  //
  // RFC 7231, Section  4.2.1:
  // ... this specification defines GET, HEAD, and POST as cacheable, ...
  //
  // So, the only methods allowed in a PUSH_PROMISE are GET and HEAD.
  SpdyHeaderBlock::const_iterator it = headers.find(":method");
  if (it == headers.end()) {
    return string();
  }
  string method_upper = it->second.as_string();
  transform(method_upper.begin(), method_upper.end(), method_upper.begin(),
            ::toupper);
  if ((method_upper != "GET" && method_upper != "HEAD")) {
    return string();
  }

  // RFC 7540, Section 8.1.2.3:
  // The ":scheme" pseudo-header field includes the scheme portion of the target
  // URI ([RFC3986], Section 3.1). ":scheme" is not restricted to "http" and
  // "https" schemed URIs.  A proxy or gateway can translate requests for
  // non-HTTP schemes, enabling the use of HTTP to interact with non-HTTP
  // services.
  //
  // Since neither Chromium nor GFE are such a proxy or gateway, ":scheme" must
  // be "http" or "https".
  it = headers.find(":scheme");
  if (it == headers.end()) {
    return string();
  }
  string scheme_lower = it->second.as_string();
  transform(scheme_lower.begin(), scheme_lower.end(), scheme_lower.begin(),
            ::tolower);
  if (scheme_lower != "http" && scheme_lower != "https") {
    return string();
  }

  // RFC 7540, Section 8.2:
  // The server MUST include a value in the ":authority" pseudo-header field for
  // which the server is authoritative (see Section 10.1).
  //
  // RFC 3986, Section 3.2
  // The authority component is preceded by a double slash ("//") and is
  // terminated by the next slash ("/"), question mark ("?"), or number sign
  // ("#") character, or by the end of the URI.
  //    authority   = [ userinfo "@" ] host [ ":" port ]
  //
  // RFC 7540, Section 8.1.2.3:
  // The authority MUST NOT include the deprecated "userinfo" subcomponent for
  // "http" or "https" schemed URIs.
  //
  // Due to RFC 3986, Section 3.2, it's implied ":authority" cannot contain
  // "/", "?", or '#". And because "userinfo" must not be present (due to
  // RFC 7540, Section 8.1.2.3), the header also cannot contain "@".
  it = headers.find(":authority");
  if (it == headers.end() || it->second.empty() ||
      it->second.find_first_of("/?#@") != SpdyStringPiece::npos) {
    return string();
  }

  string url = scheme_lower.append("://").append(it->second.as_string());

  // RFC 7540, Section 8.1.2.3:
  // The ":path" pseudo-header field includes the path and query parts of the
  // target URI (the "path-absolute" production and optionally a '?' character
  // followed by the "query" production (see Sections 3.3 and 3.4 of RFC3986).
  // A request in asterisk form includes the value '*' for the ":path"
  // pseudo-header field.
  // This pseudo-header field MUST NOT be empty for "http" or "https" URIs;
  // "http" or "https" URIs that do not contain a path MUST include a value of
  // '/'. The exception to this rule is an OPTIONS request for an "http" or
  // "https" URI that does not include a path component; these MUST include a
  // ":path" pseudo-header with a value of '*' (see RFC7230, Section 5.3.4).
  //
  // In addition to the above restriction from RFC 7540, note that RFC3986
  // defines the "path-absolute" construction as starting with a '/'
  // character. Also, OPTIONS requests cannot be pushed, so the exception
  // mentioned above does not apply here (i.e. ":path" can't be "*").
  it = headers.find(":path");
  if (it == headers.end() || it->second.empty() || it->second[0] != '/') {
    return string();
  }

  url.append(it->second.as_string());

  // |url| will now be canonicalized. The checks performed above are designed
  // so that it's impossible for a correctly-implemented URI canonicalizer to
  // internally partition |url| into scheme, authority, and path in a different
  // way than what the ":scheme", ":authority", and ":path" headers indicate.
  //
  // Examples of malicious input:
  //
  // ":scheme":    "https"
  // ":authority": "goo"
  // ":path":      "gle.com/path"
  // This method would bail on this input since ":path" does not start with "/".
  //
  // ":scheme":    ""
  // ":authority": "https://www.google.com"
  // ":path":      ""
  // This method would would bail on this input since ":authority" contains a
  // "/", and ":scheme" is not "http" or "https".
  return QuicUrlUtils::CanonicalizeUrl(url);
}

// static
string SpdyUtils::GetHostNameFromHeaderBlock(const SpdyHeaderBlock& headers) {
  // TODO(fayang): Consider just checking out the value of the ":authority" key
  // in headers.
  return QuicUrlUtils::HostName(GetPromisedUrlFromHeaderBlock(headers));
}

// static
bool SpdyUtils::UrlIsValid(const SpdyHeaderBlock& headers) {
  string url(GetPromisedUrlFromHeaderBlock(headers));
  return !url.empty() && QuicUrlUtils::IsValidUrl(url);
}

// static
bool SpdyUtils::PopulateHeaderBlockFromUrl(const string url,
                                           SpdyHeaderBlock* headers) {
  (*headers)[":method"] = "GET";
  size_t pos = url.find("://");
  if (pos == string::npos) {
    return false;
  }
  (*headers)[":scheme"] = url.substr(0, pos);
  size_t start = pos + 3;
  pos = url.find("/", start);
  if (pos == string::npos) {
    (*headers)[":authority"] = url.substr(start);
    (*headers)[":path"] = "/";
    return true;
  }
  (*headers)[":authority"] = url.substr(start, pos - start);
  (*headers)[":path"] = url.substr(pos);
  return true;
}

}  // namespace net
