// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth.h"
#include "net/http/http_auth_scheme_set.h"
#include "net/http/http_util.h"

namespace net {

HttpAuthSchemeSet::HttpAuthSchemeSet() {}

HttpAuthSchemeSet::~HttpAuthSchemeSet() {}

void HttpAuthSchemeSet::Add(const std::string& scheme) {
  DCHECK(HttpAuth::IsValidNormalizedScheme(scheme));
  schemes_.insert(scheme);
}

bool HttpAuthSchemeSet::Contains(const std::string& scheme) const {
  DCHECK(HttpAuth::IsValidNormalizedScheme(scheme));
  auto iter = schemes_.find(scheme);
  return iter != schemes_.end();
}

}  // namespace
