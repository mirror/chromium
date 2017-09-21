// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "build/build_config.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/cert/cert_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/quic/platform/api/quic_ptr_util.h"
#include "net/ssl/channel_id_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_interceptor.h"

#include "net/tools/quic/quic_http_response_proxy.h"
#include "net/tools/quic/quic_proxy_backend_url_request.h"

namespace net {

QuicHttpResponseProxy::QuicHttpResponseProxy()
    : context_(nullptr), thread_initialized_(false), proxy_thread_(nullptr) {}

QuicHttpResponseProxy::~QuicHttpResponseProxy() {
  backend_request_handlers_map_.clear();
  thread_initialized_ = false;
  if (proxy_thread_ != nullptr) {
    QUIC_LOG(INFO) << "QUIC Proxy thread: " << proxy_thread_->thread_name()
                   << " has stopped !";
    proxy_thread_.reset();
  }
}

bool QuicHttpResponseProxy::Initialize(std::string& backend_url) {
  if (!ValidateBackendUrl(backend_url)) {
    return false;
  }
  if (proxy_thread_ == nullptr) {
    proxy_thread_.reset(new base::Thread("quic proxy thread"));
    base::Thread::Options options;
    options.message_loop_type = base::MessageLoop::TYPE_IO;
    bool result = proxy_thread_->StartWithOptions(options);
    CHECK(result);
  }
  thread_initialized_ = true;
  return true;
}

bool QuicHttpResponseProxy::ValidateBackendUrl(std::string& backend_url) {
  GURL url(backend_url);
  if (!url.is_valid()) {
    QUIC_LOG(WARNING) << "QUIC Proxy Backend URL '" << backend_url
                      << "' is not valid !";
    return false;
  }
  // Remove trailing slash
  if (backend_url.back() == '/') {
    backend_url.pop_back();
  }
  backend_url_ = backend_url;

  QUIC_LOG(INFO)
      << "Successfully configured to run as a QUIC Proxy with Backend URL: "
      << backend_url_;
  return true;
}

net::URLRequestContext* QuicHttpResponseProxy::GetURLRequestContext() {
  // Access to URLRequestContext is only available on Backend Thread
  DCHECK(GetProxyTaskRunner()->BelongsToCurrentThread());
  if (context_ == nullptr) {
    net::URLRequestContextBuilder context_builder;
    // Quic reverse proxy does not cache HTTP objects
    context_builder.DisableHttpCache();
    // Enable HTTP2, but disable QUIC on the backend
    context_builder.SetSpdyAndQuicEnabled(true /* http2 */, false /* quic */);

#if defined(OS_LINUX)
    // On Linux, use a fixed ProxyConfigService, since the default one
    // depends on glib.
    context_builder.set_proxy_config_service(
        base::MakeUnique<net::ProxyConfigServiceFixed>(net::ProxyConfig()));
#endif

    // Disable net::CookieStore and net::ChannelIDService.
    context_builder.SetCookieAndChannelIdStores(nullptr, nullptr);
    context_ = context_builder.Build();
  }
  return context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
QuicHttpResponseProxy::GetProxyTaskRunner() const {
  if (proxy_thread_.get()) {
    return proxy_thread_->task_runner();
  }
  return nullptr;
}

bool QuicHttpResponseProxy::Initialized() const {
  return thread_initialized_;
}

QuicProxyBackendUrlRequest*
QuicHttpResponseProxy::CreateQuicProxyRequestHandler() {
  QuicProxyBackendUrlRequest* handler = new QuicProxyBackendUrlRequest(this);
  {
    QuicWriterMutexLock lock(&backend_handler_mutex_);
    auto inserted = backend_request_handlers_map_.insert(
        std::make_pair(handler, QuicWrapUnique(handler)));
    DCHECK(inserted.second);
  }
  return handler;
}

void QuicHttpResponseProxy::DestroyQuicProxyRequestHandler(
    QuicProxyBackendUrlRequest* proxy_request_handler) {
  if (proxy_request_handler == nullptr) {
    return;
  }
  // Cleanup the handler on the proxy thread, since it owns the url_request
  if (!GetProxyTaskRunner()->BelongsToCurrentThread()) {
    GetProxyTaskRunner()->PostTask(
        FROM_HERE,
        base::Bind(&QuicHttpResponseProxy::DestroyQuicProxyRequestHandler,
                   base::Unretained(this), proxy_request_handler));
  } else {
    // Aquire write lock and cancel if the request is still pending
    QuicWriterMutexLock lock(&backend_handler_mutex_);
    ProxyRequestHandlerMap::iterator it =
        backend_request_handlers_map_.find(proxy_request_handler);
    if (it != backend_request_handlers_map_.end()) {
      it->second->CancelRequest();
    }
    QUIC_DVLOG(1) << " Quic Proxy cleaned-up backend handler on context/main "
                     "thread for quic_conn_id: "
                  << proxy_request_handler->quic_connection_id()
                  << " quic_stream_id: "
                  << proxy_request_handler->quic_stream_id();
    size_t erased = backend_request_handlers_map_.erase(proxy_request_handler);
    DCHECK_EQ(1u, erased);
  }
}

}  // namespace net
