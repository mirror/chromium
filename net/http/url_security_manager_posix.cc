// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/memory/ptr_util.h"
#include "net/http/url_security_manager.h"

#include "net/http/http_auth_filter.h"

namespace net {

// static
std::unique_ptr<URLSecurityManager> URLSecurityManager::Create() {
  return base::MakeUnique<URLSecurityManagerWhitelist>();
}

}  //  namespace net
