// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOCK_NAVIGATION_CLIENT_H
#define MOCK_NAVIGATION_CLIENT_H

#include "content/common/navigation.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class MockNavigationClientImpl : public mojom::NavigationClient {
 public:
  MockNavigationClientImpl(mojom::NavigationClientRequest request);
  ~MockNavigationClientImpl() override;

 private:
  mojo::Binding<NavigationClient> navigation_client_binding_;
};

}  // namespace content

#endif
