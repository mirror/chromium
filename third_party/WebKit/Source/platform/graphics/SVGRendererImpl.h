// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGRendererImpl_h
#define SVGRendererImpl_h

#include "platform/PlatformExport.h"
#include "public/platform/modules/imagebitmap/SVGRenderer.mojom-blink.h"

namespace blink {

class PLATFORM_EXPORT SVGRendererImpl {
 public:
  explicit SVGRendererImpl();
  ~SVGRendererImpl() {}

 private:
  mojom::blink::SVGRendererPtr service_;
};

}  // namespace blink

#endif  // SVGRendererImpl_h
