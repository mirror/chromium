// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_
#define EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"

namespace base {
class FilePath;
class Time;
}

namespace content {
class RenderFrameHost;
}

namespace net {
class HttpResponseHeaders;
}

class GURL;

namespace extensions {
class InfoMap;

// A protocol handler for the chrome-extension:// scheme, which handles serving
// resources from disk.
class ExtensionProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  ExtensionProtocolHandler(bool is_incognito, InfoMap* extension_info_map);
  ~ExtensionProtocolHandler() override;

  // net::URLRequestJobFactory::ProtocolHandler:
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  using TestHandler =
      base::RepeatingCallback<void(base::FilePath* directory_path,
                                   base::FilePath* relative_path)>;
  // Allows tests to set a special handler for chrome-extension:// urls. Note
  // that this goes through all the normal security checks; it's essentially a
  // way to map extra resources to be included in extensions.
  static void SetTestHandler(TestHandler* handler);

 private:
  const bool is_incognito_;
  InfoMap* const extension_info_map_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionProtocolHandler);
};

// Builds HTTP headers for an extension request. Hashes the time to avoid
// exposing the exact user installation time of the extension.
scoped_refptr<net::HttpResponseHeaders> BuildHttpHeaders(
    const std::string& content_security_policy,
    bool send_cors_header,
    const base::Time& last_modified_time);

// Creates a new network::mojom::URLLoaderFactory implementation suitable for
// handling navigation requests to extension URLs.
std::unique_ptr<network::mojom::URLLoaderFactory>
CreateExtensionNavigationURLLoaderFactory(content::RenderFrameHost* frame_host);

// Attempts to create a network::mojom::URLLoaderFactory implementation suitable
// for handling subresource requests for extension URLs from |frame_host|. May
// return null if |frame_host| is never allowed to load extension subresources
// from its current navigation URL.
std::unique_ptr<network::mojom::URLLoaderFactory>
MaybeCreateExtensionSubresourceURLLoaderFactory(
    content::RenderFrameHost* frame_host,
    const GURL& frame_url);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_
