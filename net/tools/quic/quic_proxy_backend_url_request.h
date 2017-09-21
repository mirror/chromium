// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The QuicProxyBackendUrlRequest instance manages an instance of
// net::URLRequest to initiate a single HTTP call to the backend. It also
// implements the callbacks of net::URLRequest to receive the response. It is
// instantiated by a delegate (for instance, the QuicSimpleServerStream class)
// when a complete HTTP request is received within a single QUIC stream.
// However, the instance is owned by QuicHttpResponseProxy, that destroys it
// safely on the quic proxy thread. Upon receiving a response (success or
// failed), the response headers and body are posted back to the main thread. In
// the main thread, the QuicProxyBackendUrlRequest instance calls the interface,
// that is implemented by the delegate to return the response headers and body.
// In addition to managing the HTTP request/response to the backend, it
// translates the quic_spdy headers to/from HTTP headers for the backend.
//

#ifndef QUIC_PROXY_BACKEND_URL_REQUEST_H_
#define QUIC_PROXY_BACKEND_URL_REQUEST_H_

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "net/base/request_priority.h"
#include "net/base/upload_data_stream.h"
#include "net/url_request/url_request.h"

#include "net/quic/platform/api/quic_map_util.h"
#include "net/quic/platform/api/quic_string_piece.h"
#include "net/spdy/core/spdy_header_block.h"

#include "net/tools/quic/quic_http_response_proxy.h"

namespace base {
class SequencedTaskRunner;
class SingleThreadTaskRunner;
}  // namespace base

namespace net {
class HttpRequestHeaders;
class SSLCertRequestInfo;
class SSLInfo;
class UploadDataStream;

class QuicHttpResponseProxy;
class QuicSimpleServerStream;

// This class receives the callbacks during request/response calls to the
// backend
class QuicProxyDelegate {
 public:
  virtual ~QuicProxyDelegate() {}

  // Called when the response is ready at the backend and can be send back to
  // the quic client
  virtual void OnResponseBackendComplete() = 0;
};

// An adapter for making http reqs to net::URLRequest.
class QuicProxyBackendUrlRequest : public net::URLRequest::Delegate {
 public:
  enum ResponseStatus {
    SUCCESSFUL_RESPONSE,   // Received headers and/or body from backend.
    BACKEND_ERR_RESPONSE,  // No response (TCP level errors) from backend
                           // server.
  };

  // Container for response header/body pairs.
  class Response {
   public:
    Response();
    ~Response();

    ResponseStatus response_status() const { return response_status_; }
    int response_code() const { return response_code_; }
    const SpdyHeaderBlock& headers() const { return headers_; }
    const SpdyHeaderBlock& trailers() const { return trailers_; }
    const QuicStringPiece body() const { return QuicStringPiece(body_); }

    void set_response_status(ResponseStatus status) {
      response_status_ = status;
    }
    void set_response_code(int response_code) {
      response_code_ = response_code;
    }
    void set_headers(net::HttpResponseHeaders* headers,
                     uint64_t response_decoded_body_size);
    void set_headers(SpdyHeaderBlock headers) { headers_ = std::move(headers); }
    void set_trailers(SpdyHeaderBlock trailers) {
      trailers_ = std::move(trailers);
    }
    void set_body(QuicStringPiece body) {
      body_.assign(body.data(), body.size());
    }

   private:
    ResponseStatus response_status_;
    int response_code_;
    SpdyHeaderBlock headers_;
    SpdyHeaderBlock trailers_;
    std::string body_;
    bool headers_set_;
    DISALLOW_COPY_AND_ASSIGN(Response);
  };

  QuicProxyBackendUrlRequest(QuicHttpResponseProxy* context);
  ~QuicProxyBackendUrlRequest() override;

  static const std::set<std::string> kHopHeaders;
  static const int kBufferSize;
  static const int kProxyHttpBackendError;
  static const std::string kDefaultQuicPeerIP;

  // Set callbacks to be called from this to the main (quic) thread.
  // A delegate may not be set.
  // If this is called multiple times, only the last delegate will be used.
  void set_delegate(QuicProxyDelegate* delegate);
  void reset_delegate() { delegate_ = nullptr; }

  void Initialize(net::QuicConnectionId quic_connection_id,
                  net::QuicStreamId quic_stream_id,
                  std::string quic_peer_ip);

  virtual bool SendRequestToBackend(SpdyHeaderBlock* incoming_request_headers,
                                    const std::string& incoming_body);

  net::QuicConnectionId quic_connection_id() const {
    return quic_connection_id_;
  }
  net::QuicStreamId quic_stream_id() const { return quic_stream_id_; }

  net::HttpRequestHeaders* request_headers() { return &request_headers_; }
  // Releases all resources for the request and deletes the object itself.
  virtual void CancelRequest();

  // net::URLRequest::Delegate implementations:
  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) override;
  void OnSSLCertificateError(net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

  bool ResponseIsCompleted() const { return response_completed_; }
  virtual Response* GetResponse() const;

 private:
  void StartOnBackendThread();
  void SendRequestOnBackendThread();
  void ReadOnceTask();
  void OnResponseCompleted();
  void CopyHeaders(SpdyHeaderBlock* incoming_request_headers);
  bool ValidateHttpMethod(std::string method);
  bool AddRequestHeader(std::string name, std::string value);
  // Adds a request body to the request before it starts.
  void SetUpload(std::unique_ptr<net::UploadDataStream> upload);
  void SendResponseOnDelegateThread();
  void ReleaseRequest();

  // The quic proxy backend context
  QuicHttpResponseProxy* proxy_context_;
  // Send back the response from the backend to |delegate_|
  QuicProxyDelegate* delegate_;
  // Task runner for interacting with the delegate
  scoped_refptr<base::SequencedTaskRunner> delegate_task_runner_;
  // Task runner for the proxy network operations.
  scoped_refptr<base::SingleThreadTaskRunner> quic_proxy_task_runner_;

  // The corresponding QUIC conn/client/stream
  net::QuicConnectionId quic_connection_id_;
  net::QuicStreamId quic_stream_id_;
  std::string quic_peer_ip_;

  // Url, method and spec for making a http request to the Backend
  GURL url_;
  std::string method_type_;
  net::HttpRequestHeaders request_headers_;
  std::unique_ptr<net::UploadDataStream> upload_;
  std::unique_ptr<net::URLRequest> url_request_;

  // Buffers that holds the response body
  scoped_refptr<IOBuffer> buf_;
  std::string data_received_;
  bool response_completed_;
  // Response received from the backend
  std::unique_ptr<Response> quic_response_;

  base::WeakPtrFactory<QuicProxyBackendUrlRequest> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuicProxyBackendUrlRequest);
};

}  // namespace net

#endif  // QUIC_PROXY_BACKEND_URL_REQUEST_H_
