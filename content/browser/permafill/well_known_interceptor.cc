// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/permafill/browser_protocol.h"

#include <string>

#include "content/common/permafill/constants.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_redirect_job.h"
#include "url/gurl.h"

namespace permafill {

net::URLRequestJob* InterceptWellKnownURL(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  // https://<anyhost>/.well-knwon/permafill/<permafill-name>.js is redirected
  // to browser://<permafill-name>.

  const GURL& url = request->url();
  if (url.scheme() != "https")
    return nullptr;
  const std::string path = url.path();

  // Has /.well-known/permafill/ somewhere in the path? If not, skip.
  const std::string permafill_signature = "/.well-known/permafill/";
  auto permafill_offset = path.find(permafill_signature);
  if (permafill_offset == std::string::npos)
    return nullptr;
  permafill_offset += permafill_signature.size();

  // Has .js extension? If not, skip.
  if (path.size() < permafill_offset + 4 ||
      path.substr(path.size() - 3) != ".js")
    return nullptr;

  const std::string permafill_name =
      path.substr(permafill_offset, path.size() - permafill_offset - 3);

  return new net::URLRequestRedirectJob(
      request, network_delegate,
      GURL(std::string(kBrowserScheme) + "://" + permafill_name),
      net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
      "Permafill via .well-known");
}

}  // namespace permafill
