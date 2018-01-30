// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_QUIC_BACKEND_RESPONSE_H_
#define NET_TOOLS_QUIC_QUIC_BACKEND_RESPONSE_H_

#include <string>

#include "net/quic/core/spdy_utils.h"
#include "net/quic/platform/api/quic_url.h"

namespace net {  // Container for response header/body pairs.

// HTTP response returned by the QuicSimpleServerStreamBackend
class QuicBackendResponse {
 public:
  // A ServerPushInfo contains path of the push request and everything needed in
  // comprising a response for the push request.
  struct ServerPushInfo {
    ServerPushInfo(QuicUrl request_url,
                   SpdyHeaderBlock headers,
                   SpdyPriority priority,
                   std::string body);
    ServerPushInfo(const ServerPushInfo& other);
    QuicUrl request_url;
    SpdyHeaderBlock headers;
    SpdyPriority priority;
    std::string body;
  };

  enum SpecialResponseType {
    REGULAR_RESPONSE,      // Send the headers and body like a server should.
    CLOSE_CONNECTION,      // Close the connection (sending the close packet).
    IGNORE_REQUEST,        // Do nothing, expect the client to time out.
    BACKEND_ERR_RESPONSE,  // No response (TCP level errors) from backend
                           // server.
  };
  QuicBackendResponse();
  ~QuicBackendResponse();

  SpecialResponseType response_type() const { return response_type_; }
  const SpdyHeaderBlock& headers() const { return headers_; }
  const SpdyHeaderBlock& trailers() const { return trailers_; }
  const QuicStringPiece body() const { return QuicStringPiece(body_); }

  void set_response_type(SpecialResponseType response_type) {
    response_type_ = response_type;
  }

  void set_headers(SpdyHeaderBlock headers) { headers_ = std::move(headers); }
  void set_trailers(SpdyHeaderBlock trailers) {
    trailers_ = std::move(trailers);
  }
  void set_body(QuicStringPiece body) {
    body_.assign(body.data(), body.size());
  }

 private:
  SpecialResponseType response_type_;
  SpdyHeaderBlock headers_;
  SpdyHeaderBlock trailers_;
  std::string body_;

  DISALLOW_COPY_AND_ASSIGN(QuicBackendResponse);
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_QUIC_BACKEND_RESPONSE_H_
