// Copyright 2015 The Chromium Authors. All rights reserved.  Use of this source
// code is governed by a BSD-style license that can be found in the LICENSE
// file.

#include "net/http/http_auth_handler_multiround.h"

namespace net {

HttpAuthHandlerMultiRound::HttpAuthHandlerMultiRound(
    const std::string& scheme,
    scoped_ptr<Platform> platform,
    PrincipalHostNamePolicy host_name_policy,
    PrincipalPortPolicy port_policy,
    const URLSecurityManager* url_security_manager,
    HostResolver* host_resolver)
    : HttpAuthHandler(scheme),
      platform_(platform.Pass()),
      host_name_policy_(host_name_policy),
      port_policy_(port_policy),
      url_security_manager_(url_security_manager),
      host_resolver_(host_resolver) {}

HttpAuthHandlerMultiRound::~HttpAuthHandlerMultiRound() {}

} // namespace


