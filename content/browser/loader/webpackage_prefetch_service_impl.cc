// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_prefetch_service_impl.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

WebPackagePrefetchServiceImpl::WebPackagePrefetchServiceImpl(
    RenderFrameHost* render_frame_host) {}

WebPackagePrefetchServiceImpl::~WebPackagePrefetchServiceImpl() = default;

// static
void WebPackagePrefetchServiceImpl::CreateMojoService(
    RenderFrameHost* render_frame_host,
    blink::mojom::WebPackagePrefetchServiceRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<WebPackagePrefetchServiceImpl>(render_frame_host),
      std::move(request));
}

void WebPackagePrefetchServiceImpl::Prefetch(
    const GURL& url,
    blink::mojom::WebPackagePrefetchAborterRequest aborter) {
  LOG(ERROR) << "WebPackagePrefetchServiceImpl::Prefetch " << url;
}

}  // namespace content
