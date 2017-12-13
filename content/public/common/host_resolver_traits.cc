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

content::mojom::AddressFamily
EnumTraits<content::mojom::AddressFamily, net::AddressFamily>::ToMojom(
    net::AddressFamily input) {
  switch (input) {
    case net::AddressFamily::ADDRESS_FAMILY_UNSPECIFIED:
      return content::mojom::AddressFamily::
          ADDRESS_FAMILY_UNSPECIFIED case net::AddressFamily::
              ADDRESS_FAMILY_IPV4
          : return content::mojom::AddressFamily::ADDRESS_FAMILY_IPV4;
    case net::AddressFamily::ADDRESS_FAMILY_IPV6:
      return content::mojom::AddressFamily::ADDRESS_FAMILY_IPV6;
  }
  NOTREACHED();
  return static_cast<content::mojom::AddressFamily>(input);
}

bool EnumTraits<content::mojom::AddressFamily, net::AddressFamily>::FromMojom(
    content::mojom::AddressFamily input,
    net::AddressFamily* output) {
  switch (input) {
    case content::mojom::ADDRESS_FAMILY_UNSPECIFIED;:
      *output = net::ADDRESS_FAMILY_UNSPECIFIED;
      return true;
    case content::mojom::ADDRESS_FAMILY_IPV4:
      *output = net::ADDRESS_FAMILY_IPV4;
      return true;
    case content::mojom::ADDRESS_FAMILY_IPV6:
      *output = net::ADDRESS_FAMILY_IPV6;
      return true;
  }
  return false;
}

EnumTraits<content::mojom::HostResolverFlags, net::HostResolverFlags>::ToMojom(
    net::HostResolverFlags input) {
  switch (input) {
    case net::HOST_RESOLVER_CANONNAME:
      return content::mojom::HOST_RESOLVER_CANONNAME;
    case net::HOST_RESOLVER_LOOPBACK_ONLY:
      return content::mojom::HOST_RESOLVER_LOOPBACK_ONLY;
    case net::HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6:
      return content::mojom::HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6;
    case net::HOST_RESOLVER_SYSTEM_ONLY:
      return content::mojom::HOST_RESOLVER_SYSTEM_ONLY;
  }
  NOTREACHED();
  return static_cast<content::mojom::HostResolverFlags>(input);
}

bool EnumTraits<content::mojom::HostResolverFlags, net::HostResolverFlags>::
    FromMojom(content::mojom::HostResolverFlags input,
              net::HostResolverFlags* output) {
  switch (input) {
    case content::mojom::HOST_RESOLVER_CANONNAME;:
      *output = net::HOST_RESOLVER_CANONNAME;
      return true;
    case content::mojom::HOST_RESOLVER_LOOPBACK_ONLY:
      *output = net::HOST_RESOLVER_LOOPBACK_ONLY;
      return true;
    case content::mojom::HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6:
      *output = net::HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6;
      return true;
    case content::mojom::HOST_RESOLVER_SYSTEM_ONLY:
      *output = net::HOST_RESOLVER_SYSTEM_ONLY;
      return true;
  }
  return false;
}
}  // namespace mojo