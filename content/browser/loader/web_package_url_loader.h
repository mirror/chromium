// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEB_PACKAGE_URL_LOADER_H_
#define CONTENT_BROWSER_LOADER_WEB_PACKAGE_URL_LOADER_H_

#include "services/network/public/interfaces/url_loader.mojom.h"

namespace content {

class WebPackageLoader;

class WebPackageURLLoader final : public network::mojom::URLLoader {
 public:
  explicit WebPackageURLLoader(
      std::unique_ptr<WebPackageLoader> web_package_loader);
  ~WebPackageURLLoader() override;

  // network::mojom::URLLoader implementation
  void FollowRedirect() override {}
  void ProceedWithResponse() override {}
  void SetPriority(net::RequestPriority priority,
                   int intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

 private:
  std::unique_ptr<WebPackageLoader> web_package_loader_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageURLLoader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEB_PACKAGE_URL_LOADER_H_
