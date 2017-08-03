// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_COOKIE_WRAPPER_H_
#define CONTENT_PUBLIC_COMMON_COOKIE_WRAPPER_H_

#include <memory>

#include "content/public/common/cookie.mojom.h"
#include "net/cookies/cookie_store.h"

namespace content {

// Create a Mojo Cookie service wrapping a cookie store.  The caller
// must guarantee that |*cookie_store| will outlive the returned
// mojom::Cookie.
std::unique_ptr<mojom::Cookie> CreateMojoCookieWrapper(
    net::CookieStore* cookie_store);

}  // namespace content

#endif  // CONTENT_NETWORK_COOKIE_SERVICE_IMPL_H_
