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
  CanvasContextCreationAttributes(const CanvasContextCreationAttributes& other);
  virtual ~CanvasContextCreationAttributes();

  bool hasAlpha() const { return has_alpha_; }
  bool alpha() const {
    DCHECK(has_alpha_);
    return alpha_;
  }
  void setAlpha(bool);

  bool hasAntialias() const { return has_antialias_; }
  bool antialias() const {
    DCHECK(has_antialias_);
    return antialias_;
  }
  void setAntialias(bool);

  bool hasColorSpace() const { return !color_space_.IsNull(); }
  const String& colorSpace() const {
    return color_space_;
  }
  void setColorSpace(const String&);

  bool hasDepth() const { return has_depth_; }
  bool depth() const {
    DCHECK(has_depth_);
    return depth_;
  }
  void setDepth(bool);

  bool hasFailIfMajorPerformanceCaveat() const { return has_fail_if_major_performance_caveat_; }
  bool failIfMajorPerformanceCaveat() const {
    DCHECK(has_fail_if_major_performance_caveat_);
    return fail_if_major_performance_caveat_;
  }
  void setFailIfMajorPerformanceCaveat(bool);

  bool hasLowLatency() const { return has_low_latency_; }
  bool lowLatency() const {
    DCHECK(has_low_latency_);
    return low_latency_;
  }
  void setLowLatency(bool);

  bool hasPixelFormat() const { return !pixel_format_.IsNull(); }
  const String& pixelFormat() const {
    return pixel_format_;
  }
  void setPixelFormat(const String&);

  bool hasPremultipliedAlpha() const { return has_premultiplied_alpha_; }
  bool premultipliedAlpha() const {
    DCHECK(has_premultiplied_alpha_);
    return premultiplied_alpha_;
  }
  void setPremultipliedAlpha(bool);

  bool hasPreserveDrawingBuffer() const { return has_preserve_drawing_buffer_; }
  bool preserveDrawingBuffer() const {
    DCHECK(has_preserve_drawing_buffer_);
    return preserve_drawing_buffer_;
  }
  void setPreserveDrawingBuffer(bool);

  bool hasStencil() const { return has_stencil_; }
  bool stencil() const {
    DCHECK(has_stencil_);
    return stencil_;
  }
  void setStencil(bool);

  bool hasCompatibleXRDevice() const { return compatiblexr_device_; }
  EventTargetWithInlineData* compatibleXRDevice() const {
    return compatiblexr_device_;
  }
  void setCompatibleXRDevice(EventTargetWithInlineData*);

  virtual void Trace(blink::Visitor*);

 private:
  bool has_alpha_ = false;
  bool has_antialias_ = false;
  bool has_depth_ = false;
  bool has_fail_if_major_performance_caveat_ = false;
  bool has_low_latency_ = false;
  bool has_premultiplied_alpha_ = false;
  bool has_preserve_drawing_buffer_ = false;
  bool has_stencil_ = false;

  bool alpha_;
  bool antialias_;
  String color_space_;
  bool depth_;
  bool fail_if_major_performance_caveat_;
  bool low_latency_;
  String pixel_format_;
  bool premultiplied_alpha_;
  bool preserve_drawing_buffer_;
  bool stencil_;

  // This attribute is of type XRDevice, defined in modules/xr/XRDevice.h
  Member<EventTargetWithInlineData> compatiblexr_device_;
};

}  // namespace blink

#endif  // CanvasContextCreationAttributes_h
