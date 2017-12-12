// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/host_resolver_traits.h"

namespace mojo {

// static
template <>
bool StructTraits<content::mojom::HostPortPairDataView,
                  net::HostPortPair>::Read(content::mojom::HostPortPairDataView
                                               data,
                                           net::HostPortPair* out_pair) {
  std::string host = data.host();
  int16_t port = data.port();
  if (!host)
    return false;
  else
    out_pair = net::HostPortPair(data.host(), port);
  return true;
}
}  // namespace mojo