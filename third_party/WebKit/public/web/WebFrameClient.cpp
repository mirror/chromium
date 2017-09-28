// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "WebFrameClient.h"

#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

service_manager::InterfaceProvider* WebFrameClient::GetInterfaceProvider() {
  static service_manager::InterfaceProvider interface_provider;

  service_manager::mojom::InterfaceProviderPtr provider;
  mojo::MakeRequest(&provider);
  interface_provider.Bind(std::move(provider));

  return &interface_provider;
}

}  // namespace blink
