// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_MOJO_PROXY_RESOLVER_V8_TRACING_BINDINGS_H_
#define CONTENT_NETWORK_MOJO_PROXY_RESOLVER_V8_TRACING_BINDINGS_H_

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_checker.h"
//#include "content/public/common/proxy_resolver.mojom.h"
#include "net/dns/host_resolver_mojo.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy/proxy_resolver_v8_tracing.h"

namespace content {

// An implementation of ProxyResolverV8Tracing::Bindings that forwards requests
// onto a Client mojo interface. Alert() and OnError() may be called from any
// thread; when they are called from another thread, the calls are proxied to
// the origin task runner. GetHostResolver() and GetNetLogWithSource() may only
// be
// called from the origin task runner.
template <typename Client>
class MojoProxyResolverV8TracingBindings
    : public net::ProxyResolverV8Tracing::Bindings,
      public net::HostResolverMojo::Impl {
 public:
  explicit MojoProxyResolverV8TracingBindings(Client* client)
      : client_(client), host_resolver_(this) {
    DCHECK(client_);
  }

  // ProxyResolverV8Tracing::Bindings overrides.
  void Alert(const base::string16& message) override {
    DCHECK(thread_checker_.CalledOnValidThread());
    client_->Alert(base::UTF16ToUTF8(message));
  }

  void OnError(int line_number, const base::string16& message) override {
    DCHECK(thread_checker_.CalledOnValidThread());
    client_->OnError(line_number, base::UTF16ToUTF8(message));
  }

  net::HostResolver* GetHostResolver() override {
    DCHECK(thread_checker_.CalledOnValidThread());
    return &host_resolver_;
  }

  net::NetLogWithSource GetNetLogWithSource() override {
    DCHECK(thread_checker_.CalledOnValidThread());
    return net::NetLogWithSource();
  }

 private:
  // net::HostResolverMojo::Impl override.
  void ResolveDns(std::unique_ptr<net::HostResolver::RequestInfo> request_info,
                  net::interfaces::HostResolverRequestClientPtr client) {
    DCHECK(thread_checker_.CalledOnValidThread());
    client_->ResolveDns(std::move(request_info), std::move(client));
  }

  base::ThreadChecker thread_checker_;
  Client* client_;
  net::HostResolverMojo host_resolver_;
};

}  // namespace content

#endif  // CONTENT_NETWORK_MOJO_PROXY_RESOLVER_V8_TRACING_BINDINGS_H_
