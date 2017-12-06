// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_PREFETCH_SERVICE_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_PREFETCH_SERVICE_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/WebKit/public/platform/modules/webpackage/webpackage_prefetch_service.mojom.h"

namespace content {

class RenderFrameHost;
class URLLoaderFactoryGetter;

class WebPackagePrefetchServiceImpl
    : public blink::mojom::WebPackagePrefetchService {
 public:
  explicit WebPackagePrefetchServiceImpl(
      scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter);
  ~WebPackagePrefetchServiceImpl() override;

  static void CreateMojoService(
      scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter,
      blink::mojom::WebPackagePrefetchServiceRequest request);

  void Prefetch(
      const GURL& url,
      blink::mojom::WebPackagePrefetchAborterRequest aborter) override;

 private:
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_PREFETCH_SERVICE_H_
