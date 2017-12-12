// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_HOST_RESOLVER_TRAITS_H
#define CONTENT_PUBLIC_COMMON_HOST_RESOLVER_TRAITS_H

#include "content/public/common/host_resolver.mojom.h"
#include "net/base/host_port_pair.h"

namespace mojo {

template <>
struct StructTraits<content::mojom::HostPortPairDataView, net::HostPortPair> {
  static const std::string& host(const net::HostPortPair& host_port_pair) {
    return host_port_pair.host();
  }
  static uint16_t port(const net::HostPortPair& host_port_pair) {
    return host_port_pair.port();
  }
  static bool Read(content::mojom::HostPortPairDataView data,
                   net::HostPortPair* out_pair);
}
}  // namespace mojo