// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasDrawListener_h
#define CanvasDrawListener_h

#include <memory>
#include "core/CoreExport.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/RefPtr.h"
#include "public/platform/WebCanvasCaptureHandler.h"

namespace blink {

class StaticBitmapImage;

class CORE_EXPORT CanvasDrawListener : public GarbageCollectedMixin {
 public:
  virtual ~CanvasDrawListener();
  virtual void SendNewFrame(RefPtr<StaticBitmapImage>);
  bool NeedsNewFrame() const;
  void RequestFrame();

 protected:
  explicit CanvasDrawListener(std::unique_ptr<WebCanvasCaptureHandler>);

  bool frame_capture_requested_;
  std::unique_ptr<WebCanvasCaptureHandler> handler_;
};

}  // namespace blink

#endif
