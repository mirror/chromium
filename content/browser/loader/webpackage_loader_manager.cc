// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_loader_manager.h"

#include "base/logging.h"
#include "content/browser/loader/webpackage_loader.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/resource_request.h"
#include "url/gurl.h"

namespace content {

WebPackageLoaderManager::WebPackageLoaderManager(
    URLLoaderFactoryGetter* loader_factory_getter)
    : loader_factory_getter_(loader_factory_getter) {}

WebPackageLoaderManager::~WebPackageLoaderManager() {}

void WebPackageLoaderManager::Prefetch(const GURL& url) {
  LOG(ERROR) << "WebPackageLoaderManager::Prefetch " << url;
  if (loades_.count(url)) {
    LOG(ERROR) << " already exists ";
    return;
  }
  ResourceRequest resource_request;
  resource_request.url = url;
  loades_.emplace(url, std::make_unique<WebPackageLoader>(
                           resource_request, loader_factory_getter_));
}

std::unique_ptr<WebPackageLoader> WebPackageLoaderManager::TakeMatchingLoader(
    const ResourceRequest& resource_request) {
  auto it = loades_.find(resource_request.url);
  if (it == loades_.end()) {
    LOG(ERROR) << "WebPackageLoaderManager::TakeMatchingLoader not match "
               << resource_request.url;
    return std::unique_ptr<WebPackageLoader>();
  }
  LOG(ERROR) << "WebPackageLoaderManager::TakeMatchingLoader match "
             << resource_request.url;

  std::unique_ptr<WebPackageLoader> loader = std::move(it->second);
  loades_.erase(it);
  return loader;
}

}  // namespace content
