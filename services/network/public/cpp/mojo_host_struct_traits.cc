// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/mojo_host_struct_traits.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "net/base/address_list.h"
#include "services/network/public/cpp/address_family_traits.h"
#include "services/network/public/cpp/ip_endpoint_struct_traits.h"

namespace mojo {

// static
bool StructTraits<network::mojom::HostResolverRequestInfoDataView,
                  std::unique_ptr<net::HostResolver::RequestInfo>>::
    Read(network::mojom::HostResolverRequestInfoDataView data,
         std::unique_ptr<net::HostResolver::RequestInfo>* out) {
  base::StringPiece host;
  if (!data.ReadHost(&host))
    return false;

  net::AddressFamily address_family;
  if (!data.ReadAddressFamily(&address_family))
    return false;

  *out = std::make_unique<net::HostResolver::RequestInfo>(
      net::HostPortPair(host.as_string(), data.port()));
  net::HostResolver::RequestInfo& request = **out;
  request.set_address_family(address_family);
  request.set_is_my_ip_address(data.is_my_ip_address());
  return true;
}

// static
bool StructTraits<network::mojom::AddressListDataView, net::AddressList>::Read(
    network::mojom::AddressListDataView data,
    net::AddressList* out) {
  return data.ReadAddresses(&out->endpoints());
}

}  // namespace mojo
