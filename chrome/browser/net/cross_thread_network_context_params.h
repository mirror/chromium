// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CROSS_THREAD_NETWORK_CONTEXT_PARAMS_H_
#define CHROME_BROWSER_NET_CROSS_THREAD_NETWORK_CONTEXT_PARAMS_H_

#include "content/public/common/network_service.mojom.h"
#include "content/public/common/proxy_config.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_info.h"

// Utility class to move a NetworkContextParamsPtr between threads. It's needed
// because InterfacePtrs can't be passed directly between threads, instead their
// InterfacePtrInfo must be extracted. This code is only needed for the legacy
// non-network-service case, and can be removed once the network service ships.
//
// TODO(mmenke): If Mojo objects are modified to use InterfacePtrInfo instead of
// InterfacePtr, remove this class, as it will no longer be needed.
class CrossThreadNetworkContextParams {
 public:
  explicit CrossThreadNetworkContextParams(
      content::mojom::NetworkContextParamsPtr context_params);
  ~CrossThreadNetworkContextParams();

  content::mojom::NetworkContextParamsPtr ExtractParams();

 private:
  content::mojom::NetworkContextParamsPtr context_params_;
  content::mojom::ProxyConfigPollerClientPtrInfo proxy_config_poller_client_;

  DISALLOW_COPY_AND_ASSIGN(CrossThreadNetworkContextParams);
};

// Returns default set of parameters for configuring the network service.
content::mojom::NetworkContextParamsPtr CreateDefaultNetworkContextParams();

#endif  // CHROME_BROWSER_NET_CROSS_THREAD_NETWORK_CONTEXT_PARAMS_H_
