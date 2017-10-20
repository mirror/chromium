// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CROSS_THREAD_NETWORK_CONTEXT_PARAMS_H_
#define CHROME_BROWSER_NET_CROSS_THREAD_NETWORK_CONTEXT_PARAMS_H_

#include "content/public/common/network_service.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_info.h"
#include "services/proxy_resolver/public/interfaces/proxy_resolver.mojom.h"

// Utility class to move a NetworkContextParamsPtr between threads. It's needed
// because InterfacePtrs can't be passed directly between threads, instead their
// InterfacePtrInfo must be extracted. This code is only needed for the legacy
// non-network-service case, and can be removed once the network service ships.
class CrossThreadNetworkContextParams {
 public:
  explicit CrossThreadNetworkContextParams(
      content::mojom::NetworkContextParamsPtr context_params);
  ~CrossThreadNetworkContextParams();

  content::mojom::NetworkContextParamsPtr ExtractParams();

 private:
  content::mojom::NetworkContextParamsPtr context_params_;
  proxy_resolver::mojom::ProxyResolverFactoryPtrInfo proxy_resolver_factory_;

  DISALLOW_COPY_AND_ASSIGN(CrossThreadNetworkContextParams);
};

// Returns default set of parameters for configuring the network service.
content::mojom::NetworkContextParamsPtr CreateDefaultNetworkContextParams();

#endif  // CHROME_BROWSER_NET_CROSS_THREAD_NETWORK_CONTEXT_PARAMS_H_
