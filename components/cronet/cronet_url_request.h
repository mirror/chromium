// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_CRONET_URL_REQUEST_H_
#define COMPONENTS_CRONET_CRONET_URL_REQUEST_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "net/base/request_priority.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace net {
class HttpRequestHeaders;
enum LoadState;
class SSLCertRequestInfo;
class SSLInfo;
class UploadDataStream;
}  // namespace net

namespace cronet {

class CronetURLRequestContext;
class TestUtil;

// An adapter from Java CronetUrlRequest object to net::URLRequest.
// Created and configured from a Java thread. Start, ReadData, and Destroy are
// posted to network thread and all callbacks into the Java CronetUrlRequest are
// done on the network thread. Java CronetUrlRequest is expected to initiate the
// next step like FollowDeferredRedirect, ReadData or Destroy. Public methods
// can be called on any thread.
class CronetURLRequest : public net::URLRequest::Delegate {
 public:
  // Delegate implemented by owner of CronetURLReques.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OnReceivedRedirect(const std::string& new_location,
                                    int http_status_code,
                                    const std::string& http_status_text,
                                    const net::HttpResponseHeaders* headers,
                                    bool was_cached,
                                    const std::string& negotiated_protocol,
                                    const std::string& proxy_server,
                                    int64_t received_byte_count) = 0;

    virtual void OnResponseStarted(int http_status_code,
                                   const std::string& http_status_text,
                                   const net::HttpResponseHeaders* headers,
                                   bool was_cached,
                                   const std::string& negotiated_protocol,
                                   const std::string& proxy_server) = 0;

    virtual void OnReadCompleted(scoped_refptr<net::IOBuffer> buffer,
                                 int bytes_read,
                                 int64_t received_byte_count) = 0;

    virtual void OnSucceeded(int64_t received_byte_count) = 0;

    virtual void OnError(int net_error,
                         int quic_error,
                         const std::string& error_string,
                         int64_t received_byte_count) = 0;

    virtual void OnCanceled() = 0;

    virtual void OnDestroyed() = 0;

    virtual void OnStatus(int load_status) = 0;

    virtual void OnMetricsCollected(const base::Time& request_start_time,
                                    const base::TimeTicks& request_start,
                                    const base::TimeTicks& dns_start,
                                    const base::TimeTicks& dns_end,
                                    const base::TimeTicks& connect_start,
                                    const base::TimeTicks& connect_end,
                                    const base::TimeTicks& ssl_start,
                                    const base::TimeTicks& ssl_end,
                                    const base::TimeTicks& send_start,
                                    const base::TimeTicks& send_end,
                                    const base::TimeTicks& push_start,
                                    const base::TimeTicks& push_end,
                                    const base::TimeTicks& receive_headers_end,
                                    const base::TimeTicks& request_end,
                                    bool socket_reused,
                                    int64_t sent_bytes_count,
                                    int64_t received_bytes_count) = 0;
  };

  // Bypasses cache if |jdisable_cache| is true. If context is not set up to
  // use cache, |jdisable_cache| has no effect. |jdisable_connection_migration|
  // causes connection migration to be disabled for this request if true. If
  // global connection migration flag is not enabled,
  // |jdisable_connection_migration| has no effect.
  CronetURLRequest(CronetURLRequestContext* context,
                   Delegate* delegate,
                   const GURL& url,
                   net::RequestPriority priority,
                   bool disable_cache,
                   bool disable_connection_migration,
                   bool enable_metrics);
  ~CronetURLRequest() override;

  // Methods called prior to Start are never called on network thread.

  // Sets the request method GET, POST etc.
  bool SetHttpMethod(const std::string& method);

  // Adds a header to the request before it starts.
  bool AddRequestHeader(const std::string& name, const std::string& value);

  // Adds a request body to the request before it starts.
  void SetUpload(std::unique_ptr<net::UploadDataStream> upload);

  // Starts the request.
  void Start();

  void GetStatus() const;

  // Follows redirect.
  void FollowDeferredRedirect();

  // Reads more data.
  bool ReadData(net::IOBuffer* buffer, int max_bytes);

  // Releases all resources for the request and deletes the object itself.
  // |jsend_on_canceled| indicates if Java onCanceled callback should be
  // issued to indicate when no more callbacks will be issued.
  void Destroy(bool send_on_canceled);

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

 private:
  friend class TestUtil;

  void StartOnNetworkThread();
  void GetStatusOnNetworkThread() const;
  void FollowDeferredRedirectOnNetworkThread();
  void ReadDataOnNetworkThread(scoped_refptr<net::IOBuffer> read_buffer,
                               int buffer_size);
  void DestroyOnNetworkThread(bool send_on_canceled);

  // Report error and cancel request_adapter.
  void ReportError(net::URLRequest* request, int net_error);
  // Reports metrics collected.
  void MaybeReportMetrics();

  CronetURLRequestContext* context_;

  // Object that owns this CronetURLRequest.
  Delegate* delegate_;

  const GURL initial_url_;
  const net::RequestPriority initial_priority_;
  std::string initial_method_;
  int load_flags_;
  net::HttpRequestHeaders initial_request_headers_;
  std::unique_ptr<net::UploadDataStream> upload_;

  scoped_refptr<net::IOBuffer> read_buffer_;
  std::unique_ptr<net::URLRequest> url_request_;

  // Whether detailed metrics should be collected and reported to Java.
  const bool enable_metrics_;
  // Whether metrics have been reported to Java.
  bool metrics_reported_;

  DISALLOW_COPY_AND_ASSIGN(CronetURLRequest);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_CRONET_URL_REQUEST_H_
