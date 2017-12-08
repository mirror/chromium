// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_MANAGER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_MANAGER_H_

#include <map>
#include <memory>

#include "base/macros.h"

class GURL;

namespace content {

class WebPackageLoader;
struct ResourceRequest;
class URLLoaderFactoryGetter;

class WebPackageLoaderManager {
 public:
  WebPackageLoaderManager(URLLoaderFactoryGetter* loader_factory_getter);
  ~WebPackageLoaderManager();
  void Prefetch(const GURL& url);
  std::unique_ptr<WebPackageLoader> TakeMatchingLoader(
      const ResourceRequest& resource_request);

 private:
  // URLLoaderFactoryGetter owns |this|.
  URLLoaderFactoryGetter* loader_factory_getter_;
  std::map<GURL, std::unique_ptr<WebPackageLoader>> loades_;

  DISALLOW_COPY_AND_ASSIGN(WebPackageLoaderManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_LOADER_MANAGER_H_
