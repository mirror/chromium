// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_NAVIGATION_MOJO_RESPONSE_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_LOADER_NAVIGATION_MOJO_RESPONSE_URL_LOADER_FACTORY_H_

#include <memory>

#include "content/public/common/url_loader_factory.mojom.h"

namespace net {
class URLRequestContext;
}  // namespace net

namespace storage {
class FileSystemContext;
}  // namespace storage

namespace content {
class AppCacheNavigationHandleCore;
class NavigationUIData;
class ResourceContext;
class ServiceWorkerNavigationHandleCore;
struct NavigationRequestInfo;

std::unique_ptr<mojom::URLLoaderFactory>
CreateNavigationMojoResponseURLLoaderFactory(
    ResourceContext* resource_context,
    net::URLRequestContext* request_context,
    storage::FileSystemContext* upload_file_system_context,
    const NavigationRequestInfo& info,
    std::unique_ptr<NavigationUIData> navigation_ui_data,
    ServiceWorkerNavigationHandleCore* service_worker_handle_core,
    AppCacheNavigationHandleCore* appcache_handle_core);
}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_NAVIGATION_MOJO_RESPONSE_URL_LOADER_FACTORY_H_
