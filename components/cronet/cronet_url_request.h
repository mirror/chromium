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

// Wrapper around net::URLRequestContext.
// Created and configured from client thread. Start, ReadData, and Destroy are
// posted to network thread and all callbacks into the Callback() are
// done on the network thread. CronetUrlRequest client is expected to initiate
// the next step like FollowDeferredRedirect, ReadData or Destroy. Public
// methods can be called on any thread.
class CronetURLRequest {
 public:
  // Callback implemented by CronetURLRequest() caller and owned by
  // CronetURLRequest::NetworkTasks.
  class Callback {
   public:
    virtual ~Callback() = default;

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
  using OnStatusCallback = base::OnceCallback<void(int)>;

  // Bypasses cache if |disable_cache| is true. If context is not set up to
  // use cache, |disable_cache| has no effect. |disable_connection_migration|
  // causes connection migration to be disabled for this request if true. If
  // global connection migration flag is not enabled,
  // |disable_connection_migration| has no effect.
  CronetURLRequest(CronetURLRequestContext* context,
                   std::unique_ptr<Callback> callback,
                   const GURL& url,
                   net::RequestPriority priority,
                   bool disable_cache,
                   bool disable_connection_migration,
                   bool enable_metrics);

  // Methods called prior to Start are never called on network thread.

  // Sets the request method GET, POST etc.
  bool SetHttpMethod(const std::string& method);

  // Adds a header to the request before it starts.
  bool AddRequestHeader(const std::string& name, const std::string& value);

  // Adds a request body to the request before it starts.
  void SetUpload(std::unique_ptr<net::UploadDataStream> upload);

  // Starts the request.
  void Start();

  // GetStatus invokes |on_status_callback| on network thread to allow multiple
  // overlapping calls.
  void GetStatus(OnStatusCallback on_status_callback) const;

  // Follows redirect.
  void FollowDeferredRedirect();

  // Reads more data.
  bool ReadData(net::IOBuffer* buffer, int max_bytes);

  // Releases all resources for the request and deletes the object itself.
  // |send_on_canceled| indicates whether onCanceled callback should be
  // issued to indicate when no more callbacks will be issued.
  void Destroy(bool send_on_canceled);

 private:
  friend class TestUtil;

  // Private destructor invoked fron NetworkTasks::Destroy() on network thread.
  ~CronetURLRequest();

  // Network Thread performs tasks on Network Thread and owns objects that
  // live on network thread.
  class NetworkTasks : public net::URLRequest::Delegate {
   public:
    NetworkTasks(std::unique_ptr<Callback> callback,
                 const GURL& url,
                 net::RequestPriority priority,
                 int load_flags,
                 bool enable_metrics);

    // Invoked on network thread.
    ~NetworkTasks() override;

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

    void Start(CronetURLRequestContext* context,
               const std::string& method,
               std::unique_ptr<net::HttpRequestHeaders> request_headers,
               std::unique_ptr<net::UploadDataStream> upload);
    void GetStatus(OnStatusCallback on_status_callback) const;
    void FollowDeferredRedirect();
    void ReadData(scoped_refptr<net::IOBuffer> read_buffer, int buffer_size);
    void Destroy(CronetURLRequest* request, bool send_on_canceled);

    // Report error and cancel request_adapter.
    void ReportError(net::URLRequest* request, int net_error);
    // Reports metrics collected.
    void MaybeReportMetrics();

    // Callback implemented by the client.
    std::unique_ptr<CronetURLRequest::Callback> callback_;

    const GURL initial_url_;
    const net::RequestPriority initial_priority_;
    const int initial_load_flags_;

    // Whether detailed metrics should be collected and reported.
    const bool enable_metrics_;
    // Whether metrics have been reported.
    bool metrics_reported_;

    scoped_refptr<net::IOBuffer> read_buffer_;
    std::unique_ptr<net::URLRequest> url_request_;

    THREAD_CHECKER(network_thread_checker_);
    DISALLOW_COPY_AND_ASSIGN(NetworkTasks);
  };

  CronetURLRequestContext* context_;
  // |network_tasks_| is invoked on network thread.
  NetworkTasks network_tasks_;

  // Request parameters set off network thread before Start().
  std::string initial_method_;
  std::unique_ptr<net::HttpRequestHeaders> initial_request_headers_;
  std::unique_ptr<net::UploadDataStream> upload_;

  DISALLOW_COPY_AND_ASSIGN(CronetURLRequest);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_CRONET_URL_REQUEST_H_
