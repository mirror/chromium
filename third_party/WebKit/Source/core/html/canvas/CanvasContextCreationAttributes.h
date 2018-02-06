// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CanvasContextCreationAttributes_h
#define CanvasContextCreationAttributes_h

#include "core/CoreExport.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"
#include "third_party/WebKit/Source/core/dom/events/EventTarget.h"

namespace blink {

class CORE_EXPORT CanvasContextCreationAttributes {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  CanvasContextCreationAttributes();
  CanvasContextCreationAttributes(
      blink::CanvasContextCreationAttributes const&);
  virtual ~CanvasContextCreationAttributes();

  bool alpha() const { return alpha_; }
  void setAlpha(bool value) { alpha_ = value; }

  bool antialias() const { return antialias_; }
  void setAntialias(bool value) { antialias_ = value; }

  const String& colorSpace() const { return color_space_; }
  void setColorSpace(const String& value) { color_space_ = value; }

  bool depth() const { return depth_; }
  void setDepth(bool value) { depth_ = value; }

  bool failIfMajorPerformanceCaveat() const {
    return fail_if_major_performance_caveat_;
  }
  void setFailIfMajorPerformanceCaveat(bool value) {
    fail_if_major_performance_caveat_ = value;
  }

  bool lowLatency() const { return low_latency_; }
  void setLowLatency(bool value) { low_latency_ = value; }

  const String& pixelFormat() const { return pixel_format_; }
  void setPixelFormat(const String& value) { pixel_format_ = value; }
  bool premultipliedAlpha() const { return premultiplied_alpha_; }
  void setPremultipliedAlpha(bool value) { premultiplied_alpha_ = value; }

  bool preserveDrawingBuffer() const { return preserve_drawing_buffer_; }
  void setPreserveDrawingBuffer(bool value) {
    preserve_drawing_buffer_ = value;
  }

  bool stencil() const { return stencil_; }
  void setStencil(bool value) { stencil_ = value; }

  EventTargetWithInlineData* compatibleXRDevice() const {
    return compatiblexr_device_;
  }
  void setCompatibleXRDevice(EventTargetWithInlineData* value) {
    compatiblexr_device_ = value;
  }

  virtual void Trace(blink::Visitor*);

 private:
  bool alpha_ = true;
  bool antialias_ = true;
  String color_space_ = "srgb";
  bool depth_ = true;
  bool fail_if_major_performance_caveat_ = false;
  bool low_latency_ = false;
  String pixel_format_ = "8-8-8-8";
  bool premultiplied_alpha_ = true;
  bool preserve_drawing_buffer_ = false;
  bool stencil_ = false;

  // This attribute is of type XRDevice, defined in modules/xr/XRDevice.h
  Member<EventTargetWithInlineData> compatiblexr_device_;
};

}  // namespace blink

#endif  // CanvasContextCreationAttributes_h
