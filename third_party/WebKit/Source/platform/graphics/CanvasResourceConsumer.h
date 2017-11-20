// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasResourceConsumer_h
#define CanvasResourceConsumer_h

#include "platform/PlatformExport.h"
#include "platform/graphics/paint/PaintCanvas.h"

namespace blink {

class PLATFORM_EXPORT CanvasResourceConsumer {
 public:
  virtual ~CanvasResourceConsumer() {}
  virtual void NotifySurfaceInvalid() = 0;
  virtual void SetNeedsCompositingUpdate() = 0;
  virtual void RestoreCanvasMatrixClipStack(PaintCanvas*) const = 0;
  virtual void UpdateGPUMemoryUsage() = 0;
};

}  // namespace blink

#endif
