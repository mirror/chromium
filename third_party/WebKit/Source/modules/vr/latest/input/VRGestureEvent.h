// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRGestureEvent_h
#define VRGestureEvent_h

#include "modules/EventModules.h"
#include "modules/vr/latest/VRPresentationFrame.h"
#include "modules/vr/latest/input/VRGestureEventInit.h"
#include "modules/vr/latest/input/VRInputSource.h"

namespace blink {

class VRGestureEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VRGestureEvent* Create() { return new VRGestureEvent; }
  static VRGestureEvent* Create(const AtomicString& type,
                                VRPresentationFrame* frame,
                                VRInputSource* input_source) {
    return new VRGestureEvent(type, frame, input_source);
  }

  static VRGestureEvent* Create(const AtomicString& type,
                                const VRGestureEventInit& initializer) {
    return new VRGestureEvent(type, initializer);
  }

  ~VRGestureEvent() override;

  VRPresentationFrame* frame() const { return frame_.Get(); }
  VRInputSource* inputSource() const { return input_source_.Get(); }

  const AtomicString& InterfaceName() const override;

  DECLARE_VIRTUAL_TRACE();

 private:
  VRGestureEvent();
  VRGestureEvent(const AtomicString& type,
                 VRPresentationFrame*,
                 VRInputSource*);
  VRGestureEvent(const AtomicString&, const VRGestureEventInit&);

  Member<VRPresentationFrame> frame_;
  Member<VRInputSource> input_source_;
};

}  // namespace blink

#endif  // VRDisplayEvent_h
