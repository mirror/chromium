// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_IMAGE_BITMAP_SVGRENDERER_IMPL_H_
#define CONTENT_BROWSER_IMAGE_BITMAP_SVGRENDERER_IMPL_H_

#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/WebKit/public/platform/modules/imagebitmap/SVGRenderer.mojom.h"

namespace content {

class CONTENT_EXPORT SVGRendererImpl : public blink::mojom::SVGRenderer {
 public:
  SVGRendererImpl(blink::mojom::SVGRendererRequest request);
  ~SVGRendererImpl() override;
  void RenderSVG(const std::string& file_path,
                 const RenderSVGCallback& callback) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SVGRendererImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_IMAGE_BITMAP_SVGRENDERER_IMPL_H_
