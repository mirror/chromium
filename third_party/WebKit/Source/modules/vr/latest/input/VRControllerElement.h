// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRControllerElement_h
#define VRControllerElement_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRControllerElement final : public GarbageCollected<VRControllerElement>,
                                  public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRControllerElement(bool hasXAxis, bool hasYAxis)
      : hasXAxis_(hasXAxis), hasYAxis_(hasYAxis) {}

  bool pressed() const { return pressed_; }
  bool touched() const { return touched_; }
  double value() const { return value_; }
  double xAxis(bool& isNull) const {
    isNull = !hasXAxis_;
    return 0.0;
  }
  double yAxis(bool& isNull) const {
    isNull = !hasYAxis_;
    return 0.0;
  }

  bool hasAxes() const { return hasXAxis_ || hasYAxis_; }

  void setValue(double value, bool pressed, bool touched) {
    value_ = value;
    pressed_ = pressed || value_ > 0.25;
    touched_ = touched || value_ > 0.0;
  }

  void setAxes(double xAxis, double yAxis) {
    xAxis_ = xAxis;
    yAxis_ = yAxis;
  }

  DEFINE_INLINE_TRACE() {}

 private:
  const bool hasXAxis_ = false;
  const bool hasYAxis_ = false;

  bool pressed_ = false;
  bool touched_ = false;
  double value_ = 0.0;
  double xAxis_ = 0.0;
  double yAxis_ = 0.0;
};

}  // namespace blink

#endif  // VRStageBoundsPoint_h
