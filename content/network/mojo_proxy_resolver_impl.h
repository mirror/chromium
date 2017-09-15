// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_MOJO_PROXY_RESOLVER_IMPL_H_
#define CONTENT_NETWORK_MOJO_PROXY_RESOLVER_IMPL_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/common/proxy_resolver.mojom.h"
#include "net/proxy/proxy_resolver.h"

namespace net {
class ProxyResolverV8Tracing;
}  // namespace net

namespace content {

class CONTENT_EXPORT MojoProxyResolverImpl : public mojom::ProxyResolver {
 public:
  explicit MojoProxyResolverImpl(
      std::unique_ptr<net::ProxyResolverV8Tracing> resolver);

  ~MojoProxyResolverImpl() override;

 private:
  class Job;

  // mojom::ProxyResolver overrides.
  void GetProxyForUrl(const GURL& url,
                      mojom::ProxyResolverRequestClientPtr client) override;

  void DeleteJob(Job* job);

  std::unique_ptr<net::ProxyResolverV8Tracing> resolver_;
  std::map<Job*, std::unique_ptr<Job>> resolve_jobs_;

  DISALLOW_COPY_AND_ASSIGN(MojoProxyResolverImpl);
};

}  // namespace content

#endif  // CONTENT_NETWORK_MOJO_PROXY_RESOLVER_IMPL_H_
