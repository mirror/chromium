// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_SVG_RENDERER_REQUEST_FORWARDER_H_
#define CONTENT_BROWSER_RENDERER_HOST_SVG_RENDERER_REQUEST_FORWARDER_H_

#include "content/common/content_export.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "third_party/WebKit/public/platform/modules/imagebitmap/SVGRenderer.mojom.h"

namespace content {

class CONTENT_EXPORT SVGRendererRequestForwarder {
 public:
  SVGRendererRequestForwarder() {}
  ~SVGRendererRequestForwarder() {}
  static void ForwardRequest(const service_manager::BindSourceInfo& info,
                             blink::mojom::SVGRendererRequest request);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_SVG_RENDERER_REQUEST_FORWARDER_H_
