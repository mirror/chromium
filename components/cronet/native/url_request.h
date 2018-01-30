// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_
#define COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "components/cronet/cronet_url_request.h"
#include "components/cronet/cronet_url_request_context.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace net {}  // namespace net

namespace cronet {

// Implementation of Cronet_UrlRequest that uses CronetURLRequestContext.
class Cronet_UrlRequestImpl : public Cronet_UrlRequest {
 public:
  Cronet_UrlRequestImpl();
  ~Cronet_UrlRequestImpl() override;

  // Cronet_UrlRequest
  void SetContext(Cronet_UrlRequestContext context) override;
  Cronet_UrlRequestContext GetContext() override;

  void InitWithParams(Cronet_EnginePtr engine,
                      CharString url,
                      Cronet_UrlRequestParamsPtr params,
                      Cronet_UrlRequestCallbackPtr callback,
                      Cronet_ExecutorPtr executor) override;
  void Start() override;
  void FollowRedirect() override;
  void Read(Cronet_BufferPtr buffer) override;
  void Cancel() override;
  bool IsDone() override;
  void GetStatus(Cronet_UrlRequestStatusListenerPtr listener) override;

 private:
  // Callback is owned by CronetURLRequest. It is invoked and deleted
  // on the network thread.
  class Callback : public CronetURLRequest::Callback {
   public:
    explicit Callback(Cronet_UrlRequestImpl* UrlRequest,
                      Cronet_UrlRequestCallbackPtr callback,
                      Cronet_ExecutorPtr executor);
    ~Callback() override;
    // CronetURLRequest::Callback implementations:
    void OnReceivedRedirect(const std::string& new_location,
                            int http_status_code,
                            const std::string& http_status_text,
                            const net::HttpResponseHeaders* headers,
                            bool was_cached,
                            const std::string& negotiated_protocol,
                            const std::string& proxy_server,
                            int64_t received_byte_count) override;
    void OnResponseStarted(int http_status_code,
                           const std::string& http_status_text,
                           const net::HttpResponseHeaders* headers,
                           bool was_cached,
                           const std::string& negotiated_protocol,
                           const std::string& proxy_server) override;
    void OnReadCompleted(scoped_refptr<net::IOBuffer> buffer,
                         int bytes_read,
                         int64_t received_byte_count) override;
    void OnSucceeded(int64_t received_byte_count) override;
    void OnError(int net_error,
                 int quic_error,
                 const std::string& error_string,
                 int64_t received_byte_count) override;
    void OnCanceled() override;
    void OnDestroyed() override;
    void OnMetricsCollected(const base::Time& request_start_time,
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
                            int64_t received_bytes_count) override;

   private:
    // The UrlRequest which owns context that owns the callback.
    Cronet_UrlRequestImpl* url_request_ = nullptr;

    // Response info updated with number of bytes received. May be nullptr,
    // if no response has been received.
    std::unique_ptr<Cronet_UrlResponseInfo> response_info_;
    // The error reported by request. May be nullptr if no error has occurred.
    std::unique_ptr<Cronet_Error> error_;

    // Application callback interface, used by |this|.
    Cronet_UrlRequestCallbackPtr callback_ = nullptr;
    // Executor for application callback, used by |this|.
    Cronet_ExecutorPtr executor_ = nullptr;

    // All methods are invoked on the network thread.
    THREAD_CHECKER(network_thread_checker_);
    DISALLOW_COPY_AND_ASSIGN(Callback);
  };

  CronetURLRequest* request_ = nullptr;
  // Synchronize access to |request_| from different threads.
  base::Lock request_lock_;

  // TODO(mef): Create special IOBuffer wrapper.
  // Read buffer while owned by this.
  Cronet_BufferPtr read_buffer_ = nullptr;

  // URL Request context. Not owned, accessed from client thread.
  Cronet_UrlRequestContext url_request_context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Cronet_UrlRequestImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_
