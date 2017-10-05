// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth_multilogin_result.h"

OAuthMultiloginResult::Cookie::Cookie() {}

OAuthMultiloginResult::Cookie::~Cookie() {}

OAuthMultiloginResult::OAuthMultiloginResult(
    const std::string& data,
    const net::URLRequestStatus& status,
    int response_code) {
  DLOG(ERROR) << data;
}

OAuthMultiloginResult::~OAuthMultiloginResult() {}
