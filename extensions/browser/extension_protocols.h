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

namespace net {
class HttpResponseHeaders;
}

class GURL;

namespace extensions {
class ContentVerifier;
class Extension;
class ExtensionSet;
class InfoMap;
class ProcessMap;

// Helper interface definition used by the extension URLLoaderFactory to
// retrieve extensions related info. All methods are called on the UI thread.
class URLLoaderFactoryHelper {
 public:
  virtual ~URLLoaderFactoryHelper() {}

  // Look up an Extension object by id.
  virtual const Extension* GetByID(const std::string& id) const = 0;  // shared

  // Returns true if the user has allowed this extension to run in incognito
  // mode.
  virtual bool IsIncognitoEnabled(const std::string& extension_id) const = 0;

  // Return whether context associated with this helper is incognito.
  virtual bool IsOffTheRecord() const = 0;

  // Return a set of all disabled extensions.
  virtual const ExtensionSet& disabled_extensions() const = 0;

  // Return a set of all enabled extensions.
  virtual const ExtensionSet& enabled_extensions() const = 0;

  // Return the ProcessMap for the browser context associated with all requests.
  virtual const ProcessMap* process_map() const = 0;

  // Returns the unique ID for the render child process host.
  virtual int render_process_id() const = 0;

  // Determine if the requests are from a WebView.
  virtual bool IsWebViewRequest() const = 0;

  // Return the content verifier to be used for all requests.
  virtual ContentVerifier* content_verifier() const = 0;  // shared.
};

using ExtensionProtocolTestHandler =
    base::Callback<void(base::FilePath* directory_path,
                        base::FilePath* relative_path)>;

// Builds HTTP headers for an extension request. Hashes the time to avoid
// exposing the exact user installation time of the extension.
scoped_refptr<net::HttpResponseHeaders> BuildHttpHeaders(
    const std::string& content_security_policy,
    bool send_cors_header,
    const base::Time& last_modified_time);

// Creates the handlers for the chrome-extension:// scheme. Pass true for
// |is_incognito| only for incognito profiles and not for Chrome OS guest mode
// profiles.
std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateExtensionProtocolHandler(bool is_incognito, InfoMap* extension_info_map);

// Allows tests to set a special handler for chrome-extension:// urls. Note
// that this goes through all the normal security checks; it's essentially a
// way to map extra resources to be included in extensions.
void SetExtensionProtocolTestHandler(ExtensionProtocolTestHandler* handler);

// Creates a new network::mojom::URLLoaderFactory implementation suitable for
// handling navigation requests to extension URLs.
std::unique_ptr<network::mojom::URLLoaderFactory>
CreateExtensionNavigationURLLoaderFactory(
    std::unique_ptr<URLLoaderFactoryHelper> extension_helper);

// Attempts to create a network::mojom::URLLoaderFactory implementation suitable
// for handling subresource requests for extension URLs from |frame_host|.
std::unique_ptr<network::mojom::URLLoaderFactory>
CreateExtensionSubresourceURLLoaderFactory(
    const GURL& frame_url,
    std::unique_ptr<URLLoaderFactoryHelper> extension_helper);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_PROTOCOLS_H_
