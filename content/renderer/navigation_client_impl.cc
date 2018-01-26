// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/navigation_client_impl.h"

namespace content {

NavigationClientImpl::NavigationClientImpl(
    mojom::NavigationClientRequest navigation_client_request)
    : navigation_client_binding_(this, std::move(navigation_client_request)) {}

NavigationClientImpl::~NavigationClientImpl() {}

}  // namespace content
