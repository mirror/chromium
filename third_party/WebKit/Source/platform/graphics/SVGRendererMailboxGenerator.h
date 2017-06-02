// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGRendererMailboxGenerator_h
#define SVGRendererMailboxGenerator_h

#include "mojo/public/cpp/bindings/binding.h"
#include "platform/PlatformExport.h"
#include "public/platform/modules/imagebitmap/SVGRenderer.mojom-blink.h"

namespace blink {

class PLATFORM_EXPORT SVGRendererMailboxGenerator
    : public mojom::blink::SVGRenderer {
 public:
  explicit SVGRendererMailboxGenerator(mojom::blink::SVGRendererRequest request)
      : binding_(this, std::move(request)) {}
  ~SVGRendererMailboxGenerator() {}
  void RenderSVG(const WTF::String& file_path,
                 const RenderSVGCallback&) override;

 private:
  mojo::Binding<mojom::blink::SVGRenderer> binding_;
  DISALLOW_COPY_AND_ASSIGN(SVGRendererMailboxGenerator);
};

}  // namespace blink

#endif  // SVGRendererMailboxGenerator_h
