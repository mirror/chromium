// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/cookie_service.h"

#include "base/memory/ptr_util.h"
#include "content/network/cookie_service_impl.h"

namespace content {

std::unique_ptr<mojom::CookieService> CreateCookieService(
    net::CookieStore* cookie_store) {
  return base::MakeUnique<CookieServiceImpl>(cookie_store);
}

}  // namespace content
