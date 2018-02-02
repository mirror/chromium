// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_navigation_client.h"

namespace content {

MockNavigationClientImpl::MockNavigationClientImpl(
    mojom::NavigationClientRequest request)
    : navigation_client_binding_(this, std::move(request)) {}

MockNavigationClientImpl::~MockNavigationClientImpl() {}

}  // namespace content
