// Copyright 2015 The Chromium Authors. All rights reserved.  Use of this source
// code is governed by a BSD-style license that can be found in the LICENSE
// file.

#include "net/http/http_auth_handler_negotiate.h"

namespace net {

scoped_ptr<HttpAuthHandler>
HttpAuthHandlerNegotiate::Factory::CreateAuthHandlerForScheme(
    const std::string& scheme) {
  DCHECK(HttpAuth::IsValidNormalizedScheme(scheme));
  if (scheme != "negotiate")
    return scoped_ptr<HttpAuthHandler>();
  if (is_unsupported_ || auth_library_->empty())
    return scoped_ptr<HttpAuthHandler>();
  // TODO(cbentzel): Move towards model of parsing in the factory
  //                 method and only constructing when valid.
  return make_scoped_ptr(new HttpAuthHandlerNegotiate(
      auth_library_.get(), url_security_manager(), resolver_,
      disable_cname_lookup_, use_port_));
}

} // namespace net
