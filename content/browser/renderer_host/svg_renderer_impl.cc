// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/svg_renderer_impl.h"

#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

SVGRendererImpl::SVGRendererImpl(blink::mojom::SVGRendererRequest request)
    : binding_(this, std::move(request)) {}

SVGRendererImpl::~SVGRendererImpl() {}

}  // namespace content
