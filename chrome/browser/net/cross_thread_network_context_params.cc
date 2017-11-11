// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/cross_thread_network_context_params.h"

#include <utility>

#include "base/logging.h"

CrossThreadNetworkContextParams::CrossThreadNetworkContextParams(
    content::mojom::NetworkContextParamsPtr context_params) {
  DCHECK(context_params);

  proxy_config_poller_client_ =
      context_params->proxy_config_poller_client.PassInterface();
  context_params_ = std::move(context_params);
}

CrossThreadNetworkContextParams::~CrossThreadNetworkContextParams() {}

content::mojom::NetworkContextParamsPtr
CrossThreadNetworkContextParams::ExtractParams() {
  DCHECK(context_params_) << "This may only be called once.";

  context_params_->proxy_config_poller_client.Bind(
      std::move(proxy_config_poller_client_));
  return std::move(context_params_);
}
