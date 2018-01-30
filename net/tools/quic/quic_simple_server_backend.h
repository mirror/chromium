// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_QUIC_SIMPLE_SERVER_BACKEND_H
#define NET_TOOLS_QUIC_QUIC_SIMPLE_SERVER_BACKEND_H

#include "net/tools/quic/quic_backend_response.h"

namespace net {

class SpdyHeaderBlock;

// This interface implements the functionality to fetch a response
// from the backend (such as cache, http-proxy etc) to serve
// requests received by a Quic Server
class QuicSimpleServerBackend {
 public:
  // This interface implements the callback methods
  // called by the QuicSimpleServerBackend implementation
  // upon receiving a response from the backend
  class QuicServerStreamDelegate {
   public:
    virtual ~QuicServerStreamDelegate() {}

    virtual QuicConnectionId connection_id() const = 0;
    virtual QuicStreamId stream_id() const = 0;
    virtual std::string peer_host() const = 0;
    // Called when the response is ready at the backend and can be send back to
    // the QUIC client.
    virtual void OnResponseBackendComplete(
        const QuicBackendResponse* response) = 0;
    virtual void OnResponseBackendComplete(
        const QuicBackendResponse* response,
        std::list<QuicBackendResponse::ServerPushInfo> resources) = 0;
  };

  virtual ~QuicSimpleServerBackend(){};
  virtual bool InitializeBackend(const std::string& backend_url) = 0;
  virtual bool IsBackendInitialized() const = 0;
  virtual void FetchResponseFromBackend(
      QuicServerStreamDelegate* quic_server_stream,
      SpdyHeaderBlock& request_headers,
      const std::string& incoming_body) = 0;
  virtual void CloseBackendResponseStream(
      QuicServerStreamDelegate* quic_server_stream) = 0;
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_QUIC_SIMPLE_SERVER_BACKEND_H
