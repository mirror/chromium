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

 private:
  mojo::Binding<mojom::SVGRenderer> binding_;
  DISALLOW_COPY_AND_ASSIGN(SVGRendererImpl);
};

}  // namespace blink

#endif  // SVGRenderer_impl_h
