// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGRendererMailboxReceiver_h
#define SVGRendererMailboxReceiver_h

#include "platform/PlatformExport.h"
#include "public/platform/modules/imagebitmap/SVGRenderer.mojom-blink.h"

namespace blink {

class PLATFORM_EXPORT SVGRendererMailboxReceiver {
 public:
  explicit SVGRendererMailboxReceiver();
  ~SVGRendererMailboxReceiver() {}

 private:
  mojom::blink::SVGRendererPtr service_;
};

}  // namespace blink

#endif  // SVGRendererMailboxReceiver_h
