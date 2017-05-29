// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGRenderer_impl_h
#define SVGRenderer_impl_h

// #include "core/imagebitmap/SVGRenderer.mojom.h"

namespace blink {

class SVGRendererImpl : public mojom::SVGRenderer {
 public:
  explicit SVGRendererImpl(mojom::SVGRendererRequest request)
      : binding_(this, std::move(request)) {}
  ~SVGRendererImpl() {}
  // This function is supposed to ask the browser to create a full new renderer
  // process and use that process to render a SVG, then the new renderer
  // process should pass the rendered SVG back to the browser (maybe in the
  // format of a SkBitmap) and the browser will pass that back to the current
  // renderer.
  void RenderSVG(const RenderSVGCallback& callback) override {}

 private:
  mojo::Binding<mojom::SVGRenderer> binding_;
  DISALLOW_COPY_AND_ASSIGN(SVGRendererImpl);
};

}  // namespace blink

#endif  // SVGRenderer_impl_h
