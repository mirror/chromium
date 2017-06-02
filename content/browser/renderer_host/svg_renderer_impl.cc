// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/svg_renderer_impl.h"

namespace content {

// Don't bind the request, instead forward this request to a new renderer.
SVGRendererImpl::SVGRendererImpl(blink::mojom::SVGRendererRequest request) {}

SVGRendererImpl::~SVGRendererImpl() {}

}  // namespace content
