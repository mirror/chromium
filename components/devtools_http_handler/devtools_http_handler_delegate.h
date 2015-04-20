// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DEVTOOLS_HTTP_HANDLER_DEVTOOLS_HTTP_HANDLER_DELEGATE_H_
#define COMPONENTS_DEVTOOLS_HTTP_HANDLER_DEVTOOLS_HTTP_HANDLER_DELEGATE_H_

#include <string>

#include "base/files/file_path.h"

namespace devtools_http_handler {

class DevToolsHttpHandlerDelegate {
 public:
  virtual ~DevToolsHttpHandlerDelegate() {}

  // Should return discovery page HTML that should list available tabs
  // and provide attach links.
  virtual std::string GetDiscoveryPageHTML() = 0;

  // Returns frontend resource data by |path|. Only used if
  // |BundlesFrontendResources| returns |true|.
  virtual std::string GetFrontendResource(const std::string& path) = 0;
};

}  // namespace devtools_http_handler

#endif  // COMPONENTS_DEVTOOLS_HTTP_HANDLER_DEVTOOLS_HTTP_HANDLER_DELEGATE_H_
