// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_HOST_RESOLVER_H
#define CONTENT_NETWORK_HOST_RESOLVER_H

/*#include "content/public/common/host_resolver.mojom.h"*/
#include "mojo/public/cpp/bindings/binding.h"
#include "net/dns/host_resolver.h"

namespace content {
// TODO (billy.jayan): Do I need to add CONTENT_EXPORT
class HostResolver : public mojom::HostResolver {
 public:
  explicit HostResolver(net::HostResolver* host_resolver);

  ~HostResolver() override;

  void AddRequest(mojom::HostResolverRequest request);

 private:
  mojo::Binding<mojom::HostResolver> bindings_;
};
}  // namespace content
#endif  // CONTENT_NETWORK_HOST_RESOLVER_H