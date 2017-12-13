// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/ip_address_struct_traits.h"

namespace mojo {
// static
bool StructTraits<network::mojom::IPAddressDataView, net::IPAddress>::Read(
    network::mojom::IPAddressDataView data,
    net::IPAddress* out) {
  std::vector<uint8_t> bytes;
  if (!data.ReadAddressBytes(&bytes))
    return false;

  *out = net::IPAddress(bytes.data(), bytes.size());
  return out->IsValid();
}

}  // namespace mojo
