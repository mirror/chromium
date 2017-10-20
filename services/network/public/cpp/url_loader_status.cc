// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/url_loader_status.h"

namespace network {

URLLoaderStatus::URLLoaderStatus() = default;
URLLoaderStatus::URLLoaderStatus(const URLLoaderStatus& status) = default;

URLLoaderStatus::URLLoaderStatus(int error_code)
    : error_code(error_code), completion_time(base::TimeTicks::Now()) {}

URLLoaderStatus::~URLLoaderStatus() = default;

}  // namespace network
