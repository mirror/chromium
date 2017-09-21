// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The proxy functionality is implemented as a separate thread namely
// “quic proxy thread”, managed by an instance of the QuicHttpResponseProxy
// class. The QuicHttpResponseProxy instance also manages an instance of the
// class net::URLRequestContext, that manages a single context for all the
// HTTP calls made to the backend server. Finally, the QuicHttpResponseProxy
// instance owns (creates/ destroys) the instances of QuicProxyBackendUrlRequest
// to avoid orphan pointers of QuicProxyBackendUrlRequest when the corresponding
// QUIC connection is destroyed on the main thread due to several reasons. The
// QUIC connection management and protocol parsing is performed by the main/quic
// thread, in the same way as the toy QUIC server.
//
// quic_proxy_backend_url_request.h has a description of threads, the flow
// of packets in QUIC proxy in the forward and reverse directions.

#ifndef QUIC_HTTP_RESPONSE_SINGLE_BACKEND_H_
#define QUIC_HTTP_RESPONSE_SINGLE_BACKEND_H_

#include <stdint.h>

#include <memory>
#include <queue>
#include <string>

#include "base/base64.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "net/quic/platform/api/quic_mutex.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace net {
class QuicProxyBackendUrlRequest;
class URLRequestContext;

// Manages the context to proxy HTTP requests to the backend server
// Owns instance of net::URLRequestContext.
class QuicHttpResponseProxy {
 public:
  explicit QuicHttpResponseProxy();
  ~QuicHttpResponseProxy();

  // Must be called from the main (quic) thread of the quic proxy.
  bool Initialize(std::string& backend_url);
  virtual bool Initialized() const;

  QuicProxyBackendUrlRequest* CreateQuicProxyRequestHandler();
  void DestroyQuicProxyRequestHandler(
      QuicProxyBackendUrlRequest* proxy_request_handler);

  // Must be called from the backend thread of the quic proxy
  net::URLRequestContext* GetURLRequestContext();
  scoped_refptr<base::SingleThreadTaskRunner> GetProxyTaskRunner() const;

  using ProxyRequestHandlerMap =
      std::unordered_map<QuicProxyBackendUrlRequest*,
                         std::unique_ptr<QuicProxyBackendUrlRequest>>;
  const ProxyRequestHandlerMap* backend_request_handlers_map() const {
    return &backend_request_handlers_map_;
  }

  std::string backend_url() const { return backend_url_; }

 private:
  // Backend request handlers are managed by |this|
  ProxyRequestHandlerMap backend_request_handlers_map_
      GUARDED_BY(backend_handler_mutex_);

  bool ValidateBackendUrl(std::string& backend_url);

  // URLRequestContext to make URL requests to the backend
  std::unique_ptr<net::URLRequestContext> context_;  // owned by this

  bool thread_initialized_;
  // <scheme://hostname:port/ for the backend HTTP server
  std::string backend_url_;

  // Backend thread is owned by |this|
  std::unique_ptr<base::Thread> proxy_thread_;
  // Protects against concurrent access from quic (main) and proxy
  // threads for adding and clearing a backend request handler
  mutable QuicMutex backend_handler_mutex_;

  DISALLOW_COPY_AND_ASSIGN(QuicHttpResponseProxy);
};
}  // namespace net

#endif  // QUIC_HTTP_RESPONSE_SINGLE_BACKEND_H_
