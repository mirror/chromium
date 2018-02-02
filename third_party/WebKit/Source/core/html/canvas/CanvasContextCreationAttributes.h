// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/dictionary_impl.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
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
  CanvasContextCreationAttributes(blink::CanvasContextCreationAttributes const&);
  virtual ~CanvasContextCreationAttributes();

  bool alpha() const {
    return alpha_;
  }
  void setAlpha(bool);

  bool antialias() const {
    return antialias_;
  }
  void setAntialias(bool);

  const String& colorSpace() const {
    return color_space_;
  }
  void setColorSpace(const String&);

  bool depth() const {
    return depth_;
  }
  void setDepth(bool);

  bool failIfMajorPerformanceCaveat() const {
    return fail_if_major_performance_caveat_;
  }
  void setFailIfMajorPerformanceCaveat(bool);

  bool lowLatency() const {
    return low_latency_;
  }
  void setLowLatency(bool);

  const String& pixelFormat() const {
    return pixel_format_;
  }
  void setPixelFormat(const String&);

  bool premultipliedAlpha() const {
    return premultiplied_alpha_;
  }
  void setPremultipliedAlpha(bool);

  bool preserveDrawingBuffer() const {
    return preserve_drawing_buffer_;
  }
  void setPreserveDrawingBuffer(bool);

  bool stencil() const {
    return stencil_;
  }
  void setStencil(bool);

  EventTargetWithInlineData* compatibleXRDevice() const {
    return compatiblexr_device_;
  }
  void setCompatibleXRDevice(EventTargetWithInlineData*);

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
