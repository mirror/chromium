// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NAVIGATION_CLIENT_IMPL_H
#define CONTENT_RENDERER_NAVIGATION_CLIENT_IMPL_H

#include "content/common/navigation.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class NavigationClientImpl : mojom::NavigationClient {
 public:
  NavigationClientImpl(
      mojom::NavigationClientRequest navigation_client_request);
  ~NavigationClientImpl() override;

 private:
  mojo::Binding<mojom::NavigationClient> navigation_client_binding_;
};

}  // namespace content

#endif
