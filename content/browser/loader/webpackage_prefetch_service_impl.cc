// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_prefetch_service_impl.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "content/browser/loader/webpackage_loader_manager.h"
#include "content/browser/url_loader_factory_getter.h"

namespace content {

WebPackagePrefetchServiceImpl::WebPackagePrefetchServiceImpl(
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter)
    : loader_factory_getter_(loader_factory_getter) {}

WebPackagePrefetchServiceImpl::~WebPackagePrefetchServiceImpl() = default;

// static
void WebPackagePrefetchServiceImpl::CreateMojoService(
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter,
    blink::mojom::WebPackagePrefetchServiceRequest request) {
  mojo::MakeStrongBinding(std::make_unique<WebPackagePrefetchServiceImpl>(
                              std::move(loader_factory_getter)),
                          std::move(request));
}

void WebPackagePrefetchServiceImpl::Prefetch(
    const GURL& url,
    blink::mojom::WebPackagePrefetchAborterRequest aborter) {
  LOG(ERROR) << "WebPackagePrefetchServiceImpl::Prefetch " << url;
  WebPackageLoaderManager* manager =
      loader_factory_getter_->GetWebPackageLoaderManager();
  if (!manager)
    return;
  manager->Prefetch(url);
}

}  // namespace content
